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
}
