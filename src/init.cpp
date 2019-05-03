#include <Godot.hpp>
#include <gdnative_api_struct.gen.h>

#include "webcam.h"

const godot_videodecoder_interface_gdnative webcamInterface = {
		0, 1,
		nullptr,
		createPlayer,
		destroyPlayer,
		getPluginName,
		getSupportedExtensions,
		openFile,
		getLength,
		getPosition,
		seek,
		setAudioTrack,
		update,
		getVideoFrame,
		getAudioFrame,
		getChannels,
		getMixRate,
		getTextureSize
};

const godot_gdnative_ext_videodecoder_api_struct *video_api = nullptr;

extern "C" void godot_gdnative_init(godot_gdnative_init_options *o) {
	godot::Godot::gdnative_init(o);

	auto api = o->api_struct;
	for (int i = 0; i < api->num_extensions; i++) {
		switch (api->extensions[i]->type) {
			case GDNATIVE_EXT_VIDEODECODER:
				video_api = (godot_gdnative_ext_videodecoder_api_struct *) api->extensions[i];
				break;
		}
	}
}


extern "C" void godot_gdnative_terminate(godot_gdnative_terminate_options *o) {
	godot::Godot::gdnative_terminate(o);
}

extern "C" void godot_gdnative_singleton(void *handle) {
	godot::Godot::nativescript_init(handle);

	if (video_api) {
		init(video_api);
		video_api->godot_videodecoder_register_decoder(&webcamInterface);

//		godot::ResourceLoader::
//		godot::register_class<GstPlayerResource>();
	}
}
