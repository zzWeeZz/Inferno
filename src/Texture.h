#pragma once
#include "vk_engine.h"
#include "vk_types.h"

namespace vkutil
{
	bool LoadImageFromFile(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
}