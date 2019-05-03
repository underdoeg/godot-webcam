#pragma once

#include <Godot.hpp>

struct WebcamSettings{
	enum Format{
		RGB,
		MJPEG,
		H264
	};

	godot::String device = "auto";
	int width = 800;
	int height = 600;
	Format format = MJPEG;
};

class Webcam{
public:
	virtual ~Webcam(){};
	WebcamSettings settings;
	godot_real time;
	godot_pool_byte_array frame;
	virtual bool init() = 0;
	virtual void updateFrame() = 0;
};


/////////////////////////////////////////////////////////////////////////////////////////////////////
void init(const godot_gdnative_ext_videodecoder_api_struct *videoApi);

void *createPlayer(godot_object *obj);

void destroyPlayer(void *data);

const char *getPluginName();

const char **getSupportedExtensions(int *count);

godot_bool openFile(void *data, void *file);

godot_real getLength(const void *data);

godot_real getPosition(const void *data);

void seek(void *data, godot_real pos);

void setAudioTrack(void *data, godot_int track);

void update(void *data, godot_real dt);

godot_pool_byte_array *getVideoFrame(void *data);

godot_int getAudioFrame(void *data, float *, int);

godot_int getChannels(const void *data);

godot_int getMixRate(const void *data);

godot_vector2 getTextureSize(const void *data);
