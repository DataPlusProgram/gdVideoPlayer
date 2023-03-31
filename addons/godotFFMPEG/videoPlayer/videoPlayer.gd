extends Control

export(String,FILE) var videoPath = "res://addons/godotFFMPEG/videos/big_buck_bunny.mp4"
export var showOpenFileButton = false
export var showFrameButton = false
export var showFullscreenButton = false
export var autoPlay = true
export var debug = false
onready var video = $v/Video
onready var timeBar = $v/v/TimeBar
onready var timeLabel = $v/v/h/BottomBar/LabelTime

export var iconDarken = 0.4

var mouseOut = true
var pMouseOut = true
var fullscreened = false
var timebarUpdate = true
# Called when the node enters the scene tree for the first time.

onready var originalPos= rect_position
onready var originalSize= rect_size
onready var originalVideoSize = video.rect_size
onready var originalVideoPos= video.rect_position

var firstDraw = false


func _ready():
	
	get_tree().get_root().connect("size_changed", self, "windowResize")
	
	$v/v/h/BottomBar/ButtonNextFrame.visible = showFrameButton
	$v/v/h/BottomBar/ButtonPreviousFrame.visible = showFrameButton
	$v/v/h/ButtonFullscreen.visible = showFullscreenButton
	
	video.debug = debug
	playVideoFromPath(videoPath)


	
		
	

func _process(delta):
	
	if video.isPlaying:
		if timebarUpdate:
			updateTimeBar()
		updateTimeLabel()
		$v/v/h/BottomBar/ButtonPlay.pressed = false
	else:
		$v/v/h/BottomBar/ButtonPlay.pressed = true
		
		
	var mosPos = get_local_mouse_position()
	
	mouseOut = true
	
	
	
	if mosPos.x <= rect_size.x and mosPos.x > 0:
		if mosPos.y<= rect_size.y and mosPos.y > 0:
			mouseOut = false
	
	
	if fullscreened:
		if pMouseOut == true and mouseOut == false:
			if visible == false:
				$AnimationPlayer.play("fadeIn")
		
		if mouseOut == true:
			if visible == true and $v/v/h/BottomBar/ButtonAudio/VSliderVolume.visible == false:
				$AnimationPlayer.play("fadeOut")
	
	

func pause():
	video.pause()

func updateTimeBar():
	timeBar.max_value = video.getDuration()
	timeBar.value = video.getTime()
	timeBar.step = 0.01


func updateTimeLabel():
	var duration = video.getDuration()
	
	
	timeLabel.text = formatTime(video.getTime()) + " / " + formatTime(duration)

func playToggle():
	if video.isPlaying:
		video.pause()
	else:
		video.play()

func _on_ButtonPlay_pressed():
	 playToggle()




func _on_ButtonFullscreen_pressed():
	
	if !fullscreened:
		originalPos = rect_position
		originalSize = rect_size
		originalVideoSize = video.rect_size
#		originalVideoPos = video.rect_position
		
		var root = get_tree().get_root()
		$v.remove_child(video)
		
		root.add_child(video)
		root.move_child(video,0)

		setFullsize()
		

		fullscreened = true
	else:
		
		
		get_tree().get_root().remove_child(video)
		$v.add_child(video)
		$v.move_child(video,0)
		
		rect_position =originalPos
		rect_size = originalSize
		
		
		video.rect_size =originalVideoSize 
		video.rect_position = originalVideoPos

		
		fullscreened = false
	

func setFullsize():
	var parSize = get_viewport_rect().size
	video.rect_size = parSize
	
	rect_size.x = parSize.x
	rect_position.x = 0
	rect_position.y = parSize.y - $v/v.rect_size.y

func formatTime(sec):
	sec = stepify(sec,1)
	
	var hour = int(sec/3600)
	var minute =  fmod(int(sec/60),60)
	sec = fmod(sec,60.0)
	
	if hour > 0:
		return ("%s:%02d:%02d")%[hour,minute,sec]
	else:
		return ("%02d:%02d")%[minute,sec]
	


func _on_TimeBar_gui_input(event):
	
	if event is InputEventMouseMotion:
		$LabelTime.visible = true
		
		if event.position.x > $v/v/TimeBar.rect_size.x - $LabelTime.rect_size.x:
			event.position.x = $v/v/TimeBar.rect_size.x - $LabelTime.rect_size.x
		else:
			$LabelTime.rect_position.x =event.position.x
		
		var ratio = 0
		
		if rect_size.x >0:
			ratio = event.position.x/float(rect_size.x)
			
		var value = $v/v/TimeBar.max_value * ratio
		$LabelTime.text = formatTime(value)
		
	
	if event is InputEventMouseButton:
		if event.button_index != 1:
			return
			
		if event.is_pressed():
			timebarUpdate = false
			
		if !event.is_pressed():
			var t = timeBar.value
			video.seek(timeBar.value)
			timebarUpdate = true


func _on_Video_gui_input(event):
	if event.get_class() != "InputEventMouseButton":
		return
	
	if event.button_index == 1:
		if event.pressed == false:
			playToggle()
		
		
	


func _on_ButtonAudio_gui_input(event):
	if "InputEventMouseMotion":
		pass

func _on_ButtonAudio_toggled(button_pressed):
	if button_pressed:
		video.mute()
	else:
		video.unmute()


func _on_ButtonNextFrame_pressed():
	video.nextFrame()
	updateTimeBar()
	


func _on_FileDialog_file_selected(path):
	playVideoFromPath(path)
	


func _on_ButtonPreviousFrame_pressed():
	video.previousFrame()
	updateTimeBar()
	

func playVideoFromPath(path):
	video.loadVideo(path)
	
	$v/v/h/BottomBar/ButtonNextFrame.visible = showFrameButton
	$v/v/h/BottomBar/ButtonPreviousFrame.visible = showFrameButton
	$v/v/h/ButtonFullscreen.visible = showFullscreenButton
	
	if autoPlay:
		video.play()
		
	if !video.hasAudio:
		$v/v/h/BottomBar/ButtonAudio.disabled = true
	else:
		$v/v/h/BottomBar/ButtonAudio.disabled = false
		
	if !video.hasVideo:
		$v/v/h/BottomBar/ButtonNextFrame.visible = showFrameButton
		$v/v/h/BottomBar/ButtonPreviousFrame.visible = showFrameButton


func _on_ButtonAudio_mouse_entered():
	$v/v/h/BottomBar/ButtonAudio/VSliderVolume/AnimationPlayer.play("fadeIn")


func _on_ButtonAudio_mouse_exited():
	$v/v/h/BottomBar/ButtonAudio/VSliderVolume/AnimationPlayer.play("fadeOut")


func _on_VSliderVolume_mouse_entered():
	$v/v/h/BottomBar/ButtonAudio/VSliderVolume/AnimationPlayer.stop()
	$v/v/h/BottomBar/ButtonAudio/VSliderVolume.modulate.a = 1


func _on_VSliderVolume_mouse_exited():
	$v/v/h/BottomBar/ButtonAudio/VSliderVolume/AnimationPlayer.play("fadeOut")


func _on_VSliderVolume_value_changed(value):
	if value == $v/v/h/BottomBar/ButtonAudio/VSliderVolume.min_value:
		$v/v/h/BottomBar/ButtonAudio.pressed = true
	else:
		if $v/v/h/BottomBar/ButtonAudio.pressed == true:
			$v/v/h/BottomBar/ButtonAudio.pressed = false
			
		video.setVolume(value)
	
	pass # Replace with function body.


func _on_TimeBar_mouse_entered():
	$LabelTime.visible = true
	$LabelTime.modulate.a = 1
	$LabelTime/AnimationPlayer.play("fadeIn")
	
	pass # Replace with function body.


func _on_TimeBar_mouse_exited():
	$LabelTime/AnimationPlayer.play("fadeOut")
	pass # Replace with function body.





func _on_ButtonRepeat_toggled(button_pressed):
	video.looped = !button_pressed
	pass # Replace with function body.


func _on_v_mouse_exited():
	return
	if fullscreened:
		$AnimationPlayer.play("fadeOut")
	pass # Replace with function body.


func _on_v_mouse_entered():
	return
	if fullscreened:
		$AnimationPlayer.play("fadeIn")



func _on_v_gui_input(event):
	pass # Replace with function body.


func windowResize():
	if fullscreened:
		setFullsize()
	
