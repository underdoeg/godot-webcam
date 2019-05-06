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
	Godot::print_error("No Webcam implementation found.", __FUNCTION__, __FILE__, __LINE__);
#endif

	if (impl) {
		impl->image = godot::Ref<godot::Image>(godot::Image::_new());
	}
}

void Webcam::_process(float dt) {
	if (!impl) return;

	if (impl->process(dt)) {
		texture->set_data(impl->image);
	}
}

void Webcam::start() {
	if (!impl) return;

	Godot::print("Start Webcam");

	texture->create(width, height, Image::Format::FORMAT_RGB8);

	impl->open({
					   width, height,
					   device, CaptureFormat::MJPEG
			   });
}

void Webcam::stop() {
	if (!impl) return;

	Godot::print("Stop Webcam");

	impl->close();
}

void Webcam::_enter_tree() {
	if (autoStart) {
		start();
	}
}

godot::Ref<godot::ImageTexture> Webcam::getTexture() {
	return texture;
}

godot::Ref<godot::Image> Webcam::getImage() {
	if (!impl) return godot::Ref<godot::Image>();

	return impl->image;
}
