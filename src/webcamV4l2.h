#pragma once

#include <array>
#include <thread>
#include <atomic>
#include <gen/Image.hpp>

#include "webcam.h"

#ifdef V4L2_FOUND

class WebcamV4l2 : public Webcam {
	struct Buffer{
		uint8_t* start;
		size_t length;
	};

	std::array<Buffer, 4> buffers;

	int fd = -1;

	void shutdown();

	std::atomic_bool bKeepRunning;
	std::thread thread;

	godot::Image frame;

public:
	~WebcamV4l2();

	bool init() override;

	void updateFrame() override;
};

#endif