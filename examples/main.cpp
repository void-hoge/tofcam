#include <tofcam.hpp>
#include <cstdlib>
#include <cstdio>

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "usage: %s <device>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	const char* device = argv[1];
	auto camera = tofcam::Camera(device, 8);
	camera.stream_on();
	for (int i = 0; i < 120 * 5; i++) {
		auto [bytesused, index] = camera.dequeue();
		camera.enqueue(index);
	}
	camera.stream_off();
}
