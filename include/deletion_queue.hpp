#pragma once

#include <deque>
#include <functional>

namespace tracer
{

// A simple deletion queue to handle cleanup of created vulkan objects
struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function)
	{
		deletors.push_back(function);
	}

	void flush()
	{
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
		{
			(*it)(); // call the function
		}

		deletors.clear();
	}
};

} // namespace tracer
