#include <fakecam.hpp>
#include <fstream>

namespace tofcam {

FakeCamera::FakeCamera(const char* dir) {
    this->load_frames(dir);
    if (this->frames.size() == 0) {
        throw std::runtime_error("no frames");
    }else {
        fprintf(stderr, "%lu frames loaded.\n", this->frames.size());
    }
    
}

void FakeCamera::stream_on() {}
void FakeCamera::stream_off() {}

std::pair<void*, uint32_t> FakeCamera::dequeue() {
    return {this->frames[index].data(), index};
}

void FakeCamera::enqueue(const uint32_t index) {
    this->index = index >= (this->frames.size() - 1) ? 0 : index + 1;
}

std::pair<uint32_t, uint32_t> FakeCamera::get_size() const {
    return {WIDTH, HEIGHT};
}

std::pair<uint32_t, uint32_t> FakeCamera::get_bytes() const {
    return {SIZEIMAGE, BYTESPERLINE};
}

void FakeCamera::load_frames(const char* dir) {
    uint32_t count = 0;
    while (true) {
        char path[256];
        snprintf(path, sizeof(path), "%s/frame_%04d.raw", dir, count);
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) {
            break;
        }
        ifs.seekg(0, std::ios::end);
        std::streamsize size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        if (size != SIZEIMAGE) {
            continue;
        }
        this->frames.emplace_back(size);
        std::vector<uint8_t> buffer(size);
        if (!ifs.read(reinterpret_cast<char*>(this->frames.back().data()), size)) {
            break;
        }
        count++;
    }
}

}
