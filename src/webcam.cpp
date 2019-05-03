//
// Created by phwhitfield on 5/3/19.
//

#include "webcam.h"
#include "webcamV4l2.h"

#include <array>

#include <gdnative_api_struct.gen.h>

#include <ConfigFile.hpp>

using namespace godot;


const godot_gdnative_ext_videodecoder_api_struct *videoApi;

void init(const godot_gdnative_ext_videodecoder_api_struct *api) {
	videoApi = api;
}

////////////////////// utils
Webcam *p(void *p) {
	return reinterpret_cast<Webcam *>(p);
}

const Webcam *p(const void *p) {
	return reinterpret_cast<const Webcam *>(p);
}

WebcamSettings parseConfig(const String& contents){
	WebcamSettings s{};

	auto it = contents.split("\n", false);

	for(int i=0; i<it.size(); i++){
		auto splits = it[i].split("=");

		if(splits.size() == 2){
			auto key = splits[0];
			auto value = splits[1];

			if(key == "width"){
				s.width = value.to_int();
			}else if(key == "height"){
				s.height = value.to_int();
			}if(key == "device"){
				s.device = value;
			}
		}
	}

	return s;
}

///////////////////////////
void *createPlayer(godot_object *obj) {
	Godot::print("Create new webcam");

	Webcam *webcam = nullptr;

#ifdef V4L2_FOUND
	webcam = new WebcamV4l2();
#else
	Godot::print_error("No Webcam implementation found. Program will segfault in 3...2...1...CRASH!", __FUNCTION__, __FILE__, __LINE__);
#endif
	api->godot_pool_byte_array_new(&webcam->frame);
	return webcam;
}

void destroyPlayer(void *data) {
	auto cam = p(data);
	delete cam;
}

const char *getPluginName() {
	return "webcam";
}

const char **getSupportedExtensions(int *count) {
	Godot::print("Request supported extensions");
	static std::array<const char *, 1> exts = {"cam"};
	*count = exts.size();
	return exts.data();
}

godot_bool openFile(void *data, void *file) {
	auto cam = p(data);

	// read config file into a string
	String str;
	std::array<uint8_t , 4> buffer{};
	int read = 0;
	do {
		read = videoApi->godot_videodecoder_file_read(file, buffer.data(), buffer.size()-1);
		buffer[read] = '\0';
		str += String(reinterpret_cast<const char*>(buffer.data()));
	} while (read == buffer.size()-1);

	cam->settings = parseConfig(str);

	return cam->init();
}

godot_real getLength(const void *data) {
	return 0;
}

godot_real getPosition(const void *data) {
	return p(data)->time;
}

void seek(void *data, godot_real pos) {
	Godot::print("Cannot seek a webcam");
}

void setAudioTrack(void *data, godot_int track) {

}

void update(void *data, godot_real dt) {
	Godot::print("update");
	p(data)->time += dt;
}

godot_pool_byte_array *getVideoFrame(void *data) {
	Godot::print("getVideoFrame");
	auto cam = p(data);
	cam->updateFrame();
	return &cam->frame;
}

godot_int getAudioFrame(void *data, float *pcm, int numSamples) {
	Godot::print(std::string("Get Audio Frame " + std::to_string(numSamples)).c_str());
	return 0;
}

godot_int getChannels(const void *data) {
	return 1;
}

godot_int getMixRate(const void *data) {
	return 0;
}

godot_vector2 getTextureSize(const void *data) {
	return {0, 0};
}




