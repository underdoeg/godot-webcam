#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <thread>

#include <Image.hpp>

#include "webcam.h"

#ifdef V4L2_FOUND

class WebcamV4L2 : public Webcam::Implementation {
  struct Buffer {
    uint8_t *start;
    size_t length;
  };

  enum class JPEG_DECODER: int{
    INTERNAL, UJPEG, TURBOJPEG
  };

  std::thread thread{};
  std::atomic_bool bKeepRunning = false;
  std::atomic_bool bReconnect = false;
  std::atomic_bool bConnected = false;
  std::atomic_bool bNewFrame = false;
  std::atomic_int rotation = 0;
  std::mutex mtx{};
  JPEG_DECODER jpeg_decoder = JPEG_DECODER::TURBOJPEG;

  Webcam::Settings settings;

  godot::Ref<godot::Image> imageThread;

public:
  WebcamV4L2() { imageThread = godot::Ref<godot::Image>(godot::Image::_new()); }

  ~WebcamV4L2() { close(); }

  bool process(float dt) override {

    if (bReconnect) {
      bReconnect = false;
      open(settings);
      return false;
    }

    if (bNewFrame) {
      std::scoped_lock g(mtx);
      image->copy_from(imageThread);
      bNewFrame = false;
      return true;
    }
    return false;
  }

  void open(Webcam::Settings settings) override;

  void close() override {
    bKeepRunning = false;
    if (thread.joinable()) {
      thread.join();
    }
  }
};

#endif