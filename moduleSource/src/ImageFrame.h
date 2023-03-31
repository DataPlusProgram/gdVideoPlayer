#pragma once
#include <Godot.hpp>
#include <Image.hpp>

struct ImageFrame
{
	//godot::Ref<godot::Image> img;
	godot::PoolByteArray img;
	double timeStamp;
	int poolId;
};