extends Button


var initalModulate : Color
var brightModulate : Color


func _ready():
	initalModulate = modulate
	brightModulate = initalModulate + Color(.5,.5,.5,0)
	
	
	connect("mouse_entered",self,"mouseIn")
	connect("mouse_exited",self,"mouseOut")



func mouseIn():
	modulate = brightModulate
	
func mouseOut():
	modulate = initalModulate
