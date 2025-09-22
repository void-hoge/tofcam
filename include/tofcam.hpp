#pragma once

#include <vector>
#include <utility>
#include <cstdint>

namespace tofcam {

class Camera {
public:
	~Camera();

	Camera(const char* device, const uint32_t buffer_count);
private:
	int fd = -1;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t sizeimage = 0;
	std::vector<std::pair<void*, uint32_t>> buffers;

	void reset() noexcept;
};

}
