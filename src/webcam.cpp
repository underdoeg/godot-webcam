////
//// Created by phwhitfield on 5/3/19.
////
//
#include "webcam.h"
#include "webcamV4L2.h"

#include <Engine.hpp>

using namespace godot;

void Webcam::_init() {
    texture = Ref<ImageTexture>(ImageTexture::_new());

    // only create the webcam in the actual game
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

#ifdef V4L2_FOUND
    impl = std::make_shared<WebcamV4L2>();
#else
    Godot::print_error("No Webcam implementation found.", __FUNCTION__, __FILE__,
                       __LINE__);
#endif

    if (impl) {
        impl->image = godot::Ref<godot::Image>(godot::Image::_new());
    }
}

void Webcam::_process(float dt) {
    if (!impl)
        return;

    if (impl->process(dt)) {
        if (autoTexture) {
            if (impl->image.is_valid() && !impl->image->is_empty()) {

                Ref<Image> img;
                if (rotation == 1 || rotation == 2) {
                    // rotate image 90 clockwise

                    auto src = impl->image;

                    //          auto w = src->get_data().write();
                    //          auto r = src->get_data().read();
                    //          img->lock();
                    //          src->lock();
                    // memcpy(w.ptr(), r.ptr(), img->get_data().size());
                    // printf("SIZE %d", img->get_data().size());
                    auto data_src = src->get_data();
                    auto data = PoolByteArray();
                    data.resize(data_src.size());

                    auto w = data.write();

                    int image_width = src->get_width();
                    int image_height = src->get_height();

                    for (auto y = 0; y < image_height; y++) {
                        for (auto x = 0; x < image_width; x++) {

                            auto index_src = y * image_width * 3 + x * 3;

                            int x_dst = rotation == 1 ? image_height - 1 - y : y;
                            int y_dst = x;

                            auto index_dst = y_dst * image_height * 3 + x_dst * 3;
                            w[index_dst] = data_src[index_src];
                            w[index_dst + 1] = data_src[index_src + 1];
                            w[index_dst + 2] = data_src[index_src + 2];
                        }
                    }

                    img.instance();
                    img->create_from_data(image_height, image_width, false,
                                          src->get_format(), data);

                    //          src->unlock();
                    //          img->unlock();

                    //          img->lock();
                    //          src->lock();
                    //          for(auto y=0; y<src->get_height(); y++) {
                    //            for (auto x = 0; x < src->get_width(); x++) {
                    //              img->set_pixel((src->get_height() - 1) - y, x,
                    //              src->get_pixel(x, y));
                    //            }
                    //          }
                    //          src->unlock();
                    //          img->unlock();
                } else {
                    img = impl->image;
                }

                texture->set_data(img);
            }
        }
    }
}

void Webcam::start() {
    if (!impl)
        return;

    Godot::print("Start Webcam");

    if (autoTexture) {
        if (rotation == 1 || rotation == 2) {
            texture->create(height, width, Image::Format::FORMAT_RGB8);
        } else {
            texture->create(width, height, Image::Format::FORMAT_RGB8);
        }
    }

    impl->open({width, height, rotation, device, CaptureFormat::MJPEG});
}

void Webcam::stop() {
    if (!impl)
        return;

    Godot::print("Stop Webcam");

    impl->close();
}

void Webcam::_enter_tree() {
    if (autoStart) {
        start();
    }
}

void Webcam::_exit_tree() {
    stop();
}

godot::Ref<godot::ImageTexture> Webcam::getTexture() { return texture; }

godot::Ref<godot::Image> Webcam::getImage() {
    if (!impl)
        return godot::Ref<godot::Image>();

    return impl->image;
}

bool Webcam::hasImage() {
    if (!impl)
        return false;
    return impl->image->get_width() > 0;
}

