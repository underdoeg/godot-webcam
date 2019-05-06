extends Node2D


func _ready():
	$webcamView.texture = $webcam.get_texture()


func _on_saveBtn_pressed():
	$webcam.get_image().save_png("user://godot-webcam-screenshot.png")
