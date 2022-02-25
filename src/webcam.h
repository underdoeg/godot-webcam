#pragma once

#include <atomic>
#include <memory>

#include <Godot.hpp>
#include <Node.hpp>
#include <Reference.hpp>
#include <ImageTexture.hpp>
#include <EditorPlugin.hpp>

//class WebcamPlugin : public godot::EditorPlugin {
//GODOT_CLASS(WebcamPlugin, godot::EditorPlugin)
//
//public:
//	void _enter_tree() {
//		godot::Godot::print("ENTER TREE");
////		add_custom_type("MyType", "Node", "somehow load the MyType Class");
//	}
//
//	static void _register_methods() {
//		register_method("_enter_tree", &WebcamPlugin::_enter_tree);
//
//	}
//};

class Webcam : public godot::Node {
GODOT_CLASS(Webcam, godot::Node)

public:

    enum class CaptureFormat: int {
        MJPEG,
        RAW,
        H264,
    };

    struct Settings {
        int w;
        int h;
        int rotation;
        godot::String dev;
        CaptureFormat captureFormat;
    };

    class Implementation {
    public:
        godot::Ref<godot::Image> image;

        // returns true if image data has changed
        virtual bool process(float dt) = 0;

        virtual void open(Settings s) = 0;

        virtual void close() = 0;
    };

    bool autoStart = false;
    bool autoTexture = true;
    int width = 1280;
    int height = 720;
    int rotation = 0;
    godot::String device = "auto";
    // TODO: allow other formats
    int captureFormat = static_cast<int>(CaptureFormat::MJPEG);

    godot::Ref<godot::ImageTexture> texture;

    static void _register_methods() {
        register_property("auto_start", &Webcam::autoStart, false);
        register_property("auto_texture", &Webcam::autoTexture, true);
        register_property("width", &Webcam::width, 1280);
        register_property("height", &Webcam::height, 720);
        register_property("device", &Webcam::device, godot::String("auto"));
        register_property("rotation", &Webcam::rotation, 0);

        register_property("format", &Webcam::captureFormat, static_cast<int>(CaptureFormat::MJPEG),
                                       GODOT_METHOD_RPC_MODE_DISABLED, GODOT_PROPERTY_USAGE_DEFAULT,
                                       GODOT_PROPERTY_HINT_ENUM,  "mjpeg, raw");

        register_method("_process", &Webcam::_process);
        register_method("_enter_tree", &Webcam::_enter_tree);
        register_method("_exit_tree", &Webcam::_exit_tree);
        register_method("start", &Webcam::start);
        register_method("stop", &Webcam::stop);
        register_method("get_texture", &Webcam::getTexture);
        register_method("get_image", &Webcam::getImage);
        register_method("has_image", &Webcam::hasImage);


    }

    ~Webcam(){
        stop();
    }

    void _init();

    void _enter_tree();

    void _exit_tree();

    void _process(float dt);

    void _notification(int what);

    void start();

    void stop();

    bool hasImage();

    godot::Ref<godot::ImageTexture> getTexture();

    godot::Ref<godot::Image> getImage();

private:
    std::shared_ptr<Implementation> impl;
};
