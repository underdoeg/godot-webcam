////
//// Created by phwhitfield on 5/3/19.
////

#include "webcamV4L2.h"

#include <File.hpp>
#include <fcntl.h>
#include <fstream>
#include <libv4l2.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/v4l2-common.h>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <optional>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <turbojpeg.h>
#include <unistd.h>

#include "ujpeg.h"

#ifdef TURBOJPG_FOUND
#include <turbojpeg.h>
#endif

using namespace godot;

static int xioctl(int fd, int request, void *arg) {
  int r;
  do
    r = ioctl(fd, request, arg);
  while (-1 == r && EINTR == errno);
  return r;
}

void WebcamV4L2::open(Webcam::Settings s) {
  close();

  settings = s;

  thread = std::thread([&] {
    bKeepRunning = true;
    bConnected = false;

    auto reconnect = [&] {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      bReconnect = true;
    };

    std::array<Buffer, 4> buffers{};
    int fd;

    auto dev = settings.dev;

    // if device is empty or auto , try to autodetect
    if (dev == "auto" || dev.empty()) {
      for (int i = 0; i < 64; i++) {
        String path = "/dev/video" + String::num_int64(i);
        if (access(path.ascii().get_data(), F_OK) != -1) {
          dev = path;
          break;
        }
      }
    }

    if (dev == "auto") {
      Godot::print_error("Could not detect a camera", __FUNCTION__, __FILE__,
                         __LINE__);
      reconnect();
      return;
    }

    // open device
    fd = v4l2_open(dev.ascii().get_data(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
      Godot::print_error("Failed to open device '" + settings.dev + "'",
                         __FUNCTION__, __FILE__, __LINE__);
      reconnect();
      return;
    }

    // check if device is ready
    v4l2_capability capability{};
    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
      Godot::print_error("Failed to get device capabilities", __FUNCTION__,
                         __FILE__, __LINE__);
      reconnect();
      return;
    }

    // set Image format
    v4l2_format imageFormat;
    imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    imageFormat.fmt.pix.width = settings.w;
    imageFormat.fmt.pix.height = settings.h;
    switch (settings.captureFormat) {
    case Webcam::CaptureFormat::MJPEG:
      imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
      break;
    case Webcam::CaptureFormat::RAW:
      imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
      break;
    case Webcam::CaptureFormat::H264:
      imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
      break;
    }

    imageFormat.fmt.pix.field = V4L2_FIELD_NONE;
    // tell the device you are using this format
    if (ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0) {
      Godot::print_error("Failed to set capture format", __FUNCTION__, __FILE__,
                         __LINE__);
      reconnect();
      return;
    }

    v4l2_requestbuffers req{};
    req.count = buffers.size();
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
      Godot::print_error("Failed request buffers", __FUNCTION__, __FILE__,
                         __LINE__);
      reconnect();
      return;
    }

    for (unsigned i = 0; i < req.count; i++) {
      // query buffer
      v4l2_buffer buf{};
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = i;
      if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        Godot::print_error("Failed to request buffer", __FUNCTION__, __FILE__,
                           __LINE__);
        reconnect();
        return;
      }

      // map data to buffer
      auto b = v4l2_mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                         MAP_SHARED, fd, buf.m.offset);
      buffers[i].start = reinterpret_cast<uint8_t *>(b);
      if (MAP_FAILED == buffers[i].start) {
        Godot::print_error("could not map buffer", __FUNCTION__, __FILE__,
                           __LINE__);
        reconnect();
        return;
      } else {
        buffers[i].length = buf.length;
      }
    }

    for (unsigned i = 0; i < req.count; i++) {
      v4l2_buffer buf{};
      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;
      buf.index = i;
      if (xioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        Godot::print_error("Failed to exchange buffer", __FUNCTION__, __FILE__,
                           __LINE__);
        reconnect();
        return;
      }
    }

    auto type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) < 0) {
      Godot::print_error("Failed to set capture format", __FUNCTION__, __FILE__,
                         __LINE__);
      reconnect();
      return;
    }

    PoolByteArray godotBuffer;

    bConnected = true;

    std::shared_ptr<uJPEG> ujpeg;
#ifdef TURBOJPG_FOUND
    tjhandle tj;
#endif

    if (jpeg_decoder == JPEG_DECODER::UJPEG) {
      ujpeg = std::make_shared<uJPEG>();
    } else if (jpeg_decoder == JPEG_DECODER::TURBOJPEG) {
      tj = tjInitDecompress();
    }

    while (bKeepRunning) {
      fd_set fds;
      timeval tv{};
      int r;

      FD_ZERO(&fds);
      FD_SET(fd, &fds);

      /* Timeout. */
      tv.tv_sec = 1;
      tv.tv_usec = 0;

      r = select(fd + 1, &fds, NULL, NULL, &tv);

      switch (errno) {
      case EBUSY:
        Godot::print("Device is busy");
        continue;
      case EIO:
        Godot::print("Device IO Error");
        continue;
      case ENODEV:
        Godot::print("Device was unplugged, will try to repoen");
        reconnect();
        bConnected = false;
        return;
      case EBADF:
        Godot::print("Bad File number, will try to reconnect");
        reconnect();
        bConnected = false;
        return;
      }

      //			(errno == EBUSY) {
      //				Godot::print("Device is busy");
      //				continue;
      //			}

      if (r <= 0) {
        //				printf("%i\n", errno);
        continue;
      }

      v4l2_buffer buf{};

      buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buf.memory = V4L2_MEMORY_MMAP;

      // retreive buffer
      if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        continue;
      }

      auto start = buffers[buf.index].start;

      switch (settings.captureFormat) {
      case Webcam::CaptureFormat::MJPEG:

        if (godotBuffer.size() != buf.bytesused) {
          godotBuffer.resize(buf.bytesused);
        }

        // TODO: only internal decompression actually needs this godot buffer,
        // could be skipped for turbojpeg and ujpeg
        memcpy(godotBuffer.write().ptr(), start, buf.bytesused);

        //        File f;
        //        f.open("/home/phwhitfield/test.jpg", File::WRITE);
        //        f.store_buffer(godotBuffer);
        //        f.close();

        mtx.lock();

        if (jpeg_decoder == JPEG_DECODER::UJPEG) {
          ujpeg->decode(godotBuffer.read().ptr(), godotBuffer.size());

          if (!ujpeg->bad()) {
            //            if (imageThread->get_data().size() !=
            //            njGetImageSize()) {
            //              imageThread->create(njGetWidth(), njGetHeight(),
            //              false,
            //                                  Image::FORMAT_RGB8);
            //            }
            //            memcpy(imageThread->get_data().write().ptr(),
            //            njGetImage(),
            //                   njGetImageSize());
            Godot::print("NEW FRAME " +
                         String::num_int64(ujpeg->getImageSize()));
            PoolByteArray data;
            data.resize(ujpeg->getImageSize());
            memcpy(data.write().ptr(), ujpeg->getImage(),
                   ujpeg->getImageSize());
            imageThread->create_from_data(ujpeg->getWidth(), ujpeg->getHeight(),
                                          false, Image::FORMAT_RGB8, data);
            bNewFrame = true;
          } else {
            Godot::print_error("Error reading frame with nanojpg", __FUNCTION__,
                               __FILE__, __LINE__);
          }
        } else if (jpeg_decoder == JPEG_DECODER::TURBOJPEG) {
#ifdef TURBOJPG_FOUND

          int jpegSubsamp, width, height;
          tjDecompressHeader2(tj, godotBuffer.write().ptr(), godotBuffer.size(),
                              &width, &height, &jpegSubsamp);
          PoolByteArray data;
          data.resize(width * height * 3);
          if(tjDecompress2(tj, godotBuffer.write().ptr(), godotBuffer.size(),
                        data.write().ptr(), width, 0 /*pitch*/, height,
                        TJPF_RGB, TJFLAG_FASTDCT) >= 0) {
            imageThread->create_from_data(width, height, false,
                                          Image::FORMAT_RGB8, data);
            bNewFrame = true;
          }
#endif
        } else {
          if (imageThread->load_jpg_from_buffer(godotBuffer) == Error::OK) {
            bNewFrame = true;
          }
        }

        //        imageThread->save_png("/home/phwhitfield/test.png");

        mtx.unlock();
        break;
      default:
        Godot::print("Capture format not yet implemented");
      }

      // release buffer
      xioctl(fd, VIDIOC_QBUF, &buf);
    }

#ifdef TURBOJPG_FOUND
    if (jpeg_decoder == JPEG_DECODER::TURBOJPEG) {
      tjDestroy(tj);
    }
#endif

    // shutdown
    if (xioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
      Godot::print("Can not turn off webcam stream");
    }
    for (auto &buffer : buffers) {
      if (buffer.length == 0)
        continue;
      v4l2_munmap(buffer.start, buffer.length);
    }
    v4l2_close(fd);
  });
}
