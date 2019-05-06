extends Node2D


func _ready():
	$webcamView.texture = $webcam.getTexture()

