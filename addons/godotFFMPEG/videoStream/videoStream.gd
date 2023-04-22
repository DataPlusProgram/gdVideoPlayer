extends TextureRect
signal finished

export(String,FILE) var videoPath = "res://videos/big_buck_bunny.mp4"
export var spatialAudio = false
export var spatialAttenuation = 1
var image
var startTime = -1
var latestTime = 0
var frameBuffer = []
var runningAudioBuffer : PoolVector2Array = []
var audioGen : AudioStreamGeneratorPlayback
var duration = -1
var isPlaying = true
var isPaused = false
var isOver = false
var prevPoolId = -1
var curTime: float = -1
var curVidTime : float= -1
var curAudioTime : float = -1
var curVidPath = ""
var hasVideo
var hasAudio
var initialzed : bool = false
var pauseTime = 0
var gotLastFrame = false
var threadOn = false
var width
var height
var showDebug = false
var dataToProc = null
var imageToProc =null
var audioRemainder : PoolVector2Array = []
var writeAduioToFile= true
var channels
var audioRatio = -1
var videoRatio = 1
var swapBuffer = []
var receivedFirstImageFrame = false


onready var audio : Node = $audio
export var autoPlay = true
var frameBufferSize = 300
export var audioBufferLength : float = 0.5
export var dontProcessAudio : bool = false
export var looped = false
export var proccesThread = false
export var  videoBuffer : int = 10
export var debug = false
var theTexture 
var tBuffer = null
var audioThread = Thread.new()
var videoThread = Thread.new()
var playbackSpeed : float  = 1
var thread : Thread
var waitTicks : int = 8

var seeked = false

func _ready():
	
	if spatialAudio:
		makeAudioSpatial()
	
	$debug.visible = showDebug
	if proccesThread:
		thread = Thread.new()
	
	print($Node.getVersion())
	print($Node.getLicense())
	
	if videoPath != "":
		if autoPlay:
			isPlaying = autoPlay
			loadVideo(videoPath)
	
	
	
	

func makeAudioSpatial():
	yield(get_tree().root, "ready")
	audio.queue_free()
	var aud3D = AudioStreamPlayer3D.new()
	var newPar = Spatial.new()
	
	aud3D.unit_size = spatialAttenuation
	newPar.set_script(load("res://addons/godotFFMPEG/videoStream/Spatial.gd"))
	
	
	get_parent().add_child(newPar)
	get_parent().remove_child(self)
	
	aud3D.stream = audio.stream
	
	audio.queue_free()
	newPar.add_child(aud3D)
	audio = aud3D
	newPar.add_child(self)
	newPar.name = name
	reloadAudioStream()
	
func loadVideo(path):
	close()
	swapBuffer.clear()
	isOver = false
	var globalPath  = ProjectSettings.globalize_path(path)
	var ret = $Node.loadFile(globalPath,false)
	#$Node.setVideoBufferSize(videoBuffer)
	
	var err = ret["error"]
	
	if err < 0:
		print("Error opening video file:" + path)
		return
	
	audioRatio = ret["audioRatio"]
	videoRatio = ret["videoRatio"]
	

	
	var dim  =$Node.getDimensions()
	width = dim.x
	height = dim.y
	hasAudio = ret["hasAudio"]
	hasVideo = ret["hasVideo"]
	
	
	image = Image.new()
	image.create(dim.x,dim.y,false,Image.FORMAT_RGB8)
	image.lock()
	texture.image = image
	image.unlock()
	
	
	duration = $Node.getDuration()
	
	var audioInfo = $Node.getAudioInfo()
	channels = audioInfo[1]
	var smapleRate = audioInfo[2]
	var audioStream = AudioStreamGenerator.new()
	
	audioStream.mix_rate = smapleRate
	audioStream.buffer_length = audioBufferLength
	audio.stream = audioStream
	audioGen = audio.get_stream_playback()
	curVidPath = path
	initialzed = true
	isPlaying = autoPlay
	pauseTime = 0
	waitTicks = 8
	
	if proccesThread:
		threadOn = true
		videoThread.start(self,"thread_process")
	
	
 

func _input(event):
	if !debug:
		return
	var just_pressed = event.is_pressed() and not event.is_echo()
	if Input.is_key_pressed(KEY_D) and just_pressed:
		showDebug = !showDebug
		$debug.visible = showDebug


func _process(delta) -> void:
	
	if showDebug:
		var text =  "time diff: %s\n" +"localTime%s\n" +"curVidTime: %s\n" + "curAudioTime%s\n" + "videoBufferSize%s\n" + "audioBufferSize%s\n" + "imageFrame:%s\n" + "audioFrame:%s\n"
		$debug.text = text % [curVidTime-curTime,curTime,curVidTime,curAudioTime,$Node.getImageBufferSize(),$Node.getAudioBufferSize(),$Node.getImageFrameBufferSize(),$Node.getAudioFrameBufferSize()]

	if proccesThread:
		if swapBuffer.size() != 0:
			if curTime >= curVidTime:
				texture = swapBuffer.pop_front()
	
		return
	
	if !initialzed: 
		return
	
	#if waitTicks <= 0:
	#	waitTicks -= 1
	#	return
	
	if isOver and looped:
		seek(0)
		seeked = true
		if !looped:
			isPlaying = true
		
		

	
	if !isPlaying:
		if audio.playing: 
			audio.stream_paused = true
		return
	else:
		audio.stream_paused =false
	
	curVidTime = $Node.getCurVideoTime()
	curAudioTime = $Node.getCurAudioTime()
	curTime = getTime()
	var cur
	
	var behindVideo : bool = (curVidTime - curTime) <= -0.1
	var behindAudio  : bool = (curAudioTime - curTime) <= -0.1
	
	
	$Node.process()
	
	curVidTime = $Node.getCurVideoTime()

	if startTime ==-1:
		startTime = OS.get_system_time_msecs() 

	if !isOver and hasVideo:
		renderUpdate()
		
	if !dontProcessAudio and hasAudio:
		processAudio()

	
func thread_process():
	while(true):
		if threadOn == false:
			return
			
		if !initialzed: 
			OS.delay_msec(150)
			continue
		
		if isOver and looped:
			seek(0)

		
		if !isPlaying:
			OS.delay_msec(150)
			continue

		var res = $Node.process()


		if startTime ==-1:
			startTime = OS.get_system_time_msecs() 

		if !isOver and hasVideo:
			renderUpdate()
		
		if !dontProcessAudio and hasAudio:
			processAudio()


func renderUpdate() -> void:
	
	if !initialzed:
		return 
	curVidTime =  $Node.getCurVideoTime()
	curTime = getTime()# - pauseTime
	
	
	var itt = 0
	

	var imageBufferSize = $Node.getImageBufferSize()

	var b1 = curTime >= curVidTime
	var b2 = imageBufferSize > 0 or swapBuffer.size()>0
	var b3 = curVidTime == 99999
	
	if(b1 or b2):
		if swapBuffer.size() < 2:
			if imageBufferSize > 0:
				var image : Image = createImageFromBuffer()
				var t = ImageTexture.new()
				t.create_from_image(image,ImageTexture.FLAG_FILTER | ImageTexture.FLAG_VIDEO_SURFACE)
				swapBuffer.append(t)
				update()
		
		if curTime >= curVidTime:
			if swapBuffer.size() != 0:
				texture = swapBuffer.pop_front()
		
		curVidTime =  $Node.getCurVideoTime() 
		
	b1 = getTime() > duration
	b2 = duration > 0
	b3 = $Node.getImageBufferSize() == 0
	
	if b1 and b2 and b3  and swapBuffer.size() == 0:
		curVidTime = duration
		isOver = true
	else:
		isOver = false

var lastDisplayedFrame = 0

func createImageFromBuffer() -> Image:
	var data : PoolByteArray = $Node.popRawBuffer()
	var image : Image = Image.new()
	lastDisplayedFrame = curVidTime
	image.create_from_data(width,height,false,Image.FORMAT_RGB8,data)
	curVidTime = $Node.getCurVideoTime()
	
	return(image)

func processAudio() -> void:
	
	curAudioTime =  $Node.getCurAudioTime()
	curTime = getTime()# - pauseTime
	
	
	
	if (curTime <= curAudioTime) or dontProcessAudio:
		return

	var frameAvail : int = audioGen.get_frames_available()

	
	if frameAvail == 0:
		return
	
	if (curTime < curAudioTime):
		return
	
	
	if curTime > duration and !hasVideo:
		isOver = true
	
	if $Node.getAudioBufferSize() > 0  and  curTime >= curAudioTime:
		audioGen
		frameAvail = audioGen.get_frames_available()
		
		var out =$Node.popAudioBuffer()

		curAudioTime = $"Node".getCurAudioTime()
		audioGen.push_buffer(out)

	if !audio.playing:
		audio.play()


var audioOutFile

func writeAudio(raw):
	if audioOutFile == null:
		audioOutFile = File.new()
		audioOutFile.open("res://dbg/raw.dat", File.WRITE)
	
	for i in raw:
		audioOutFile.store_real(i.x)
		audioOutFile.store_real(i.y)#
		


func reloadAudioStream():
	var audioStream = AudioStreamGenerator.new()
	var audioInfo = $Node.getAudioInfo()
	var smapleRate = audioInfo[2]
	channels = audioInfo[1]
	
	audioStream.mix_rate = smapleRate
	audioStream.buffer_length = audioBufferLength
	audio.stream = audioStream
	audioGen = audio.get_stream_playback()
	
	
	#audio.stop()
	#audioGen.clear_buffer()
	return 

func seek(timeSec) -> void:
	isPlaying = false
	var gt = getTime()
	
	if audioGen != null:
		reloadAudioStream()
	
	isOver = false
	frameBuffer = []
	$Node.seek(timeSec)
	curVidTime = $Node.getCurVideoTime()
	curAudioTime = $Node.getCurAudioTime()
	
	if hasVideo:
		startTime = OS.get_system_time_msecs() - curVidTime*1000.0
	elif hasAudio:
		startTime = OS.get_system_time_msecs() - curAudioTime*1000.0
		
	if hasVideo and hasAudio and curVidTime == 0:
		startTime = OS.get_system_time_msecs() - curAudioTime*1000.0
	
	isPaused = false
	pauseTime = 0

	curTime = getTime()
	
	isOver = false
	play()
	
	 
	
func close():
	if initialzed:
		startTime = -1
		latestTime = 0
		duration = -1
		prevPoolId = -1
		curVidTime = -1
		initialzed = false
		
		
		if proccesThread:
			threadOn = false
			videoThread.wait_to_finish()
		$Node.close()
		

func getDuration():
	return $Node.getDuration()

func getTime() -> float:
	
	if isPaused:
		var pauseDiff =(OS.get_system_time_msecs() -  pauseTime) / 1000.0
		return (((OS.get_system_time_msecs() -startTime) / 1000.0)-pauseDiff)*playbackSpeed
	return ((OS.get_system_time_msecs() -startTime) / 1000.0)*playbackSpeed

var pauseStart = -1

func pause():
	if isPaused:
		return
	isPaused = true
	pauseTime = OS.get_system_time_msecs()
	isPlaying = false
	
func play():
	if isPaused:
		var pausedDur = (OS.get_system_time_msecs() - pauseTime)
		startTime += pausedDur
		pauseTime = 0
		isPaused = false
		
	isPlaying = true
	
	#startTime += (OS.get_system_time_msecs()-pauseTime)
var premuteVolume = 0
var isMute = false

func toggleMute():
	if !hasAudio:
		return
		
	if isMute:
		unmute()
	else:
		mute()

func mute():
	if !isMute:
		premuteVolume = audio.volume_db
		audio.volume_db = -INF
		audio.stream_paused = true
		reloadAudioStream()
		isMute = true
	
func unmute():
	audio.volume_db = premuteVolume
	dontProcessAudio = false
	audio.stream_paused = false
	isMute = false

func nextFrame():
	swapBuffer.clear()
	pause()
	var time = $Node.getCurVideoTime()
	
	isPlaying = false
	isPaused = false
	$Node.process()
	
	renderUpdate()
	processAudio()
	isPaused = true
	#var diff = ($Node.getCurVideoTime() + time)*1000
	startTime = OS.get_system_time_msecs() - time*1000
	pauseTime = OS.get_system_time_msecs() 
	time = getTime()
	#startTime -= diff
	curVidTime = $Node.getCurVideoTime()

var frameAdvTime = 0

func previousFrame():
	swapBuffer.clear()
	pause()
	
	var time = $Node.getCurVideoTime()

	$Node.seekPreviousFrame($Node.getCurVideoTime())
	var diff = ($Node.getCurVideoTime() - time)*1000
	startTime -= diff
	isPaused = false
	renderUpdate()
	isPaused = true
	var t = time*1000


func setVolume(db):
	$audio.volume_db = db
	pass


func getTexture():
	return texture
