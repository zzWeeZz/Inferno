﻿// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vulkan/vulkan.h>

//we will add our main reusable types here
#include <vk_mem_alloc.h>

struct AllocatedImage
{
	VkImage image;
	VmaAllocation allocation;
};

struct AllocatedBuffer
{
	VkBuffer buffer;
	VmaAllocation allocation;
};