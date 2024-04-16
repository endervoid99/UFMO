#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>


#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

//#include <fmt/core.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "spdlog/spdlog.h"
#include <vulkan/vk_enum_string_helper.h>
#include <cpptrace/cpptrace.hpp>


#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
            spdlog::error("Detected Vulkan error: {} ",string_VkResult(err));           \
			cpptrace::generate_trace().print(); 					\
			abort();                                                \
		}                                                           \
	} while (0)