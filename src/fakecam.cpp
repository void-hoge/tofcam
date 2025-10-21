#include <fakecam.hpp>
#include <fstream>

namespace tofcam {

FakeCamera::FakeCamera(const char* dir, const uint32_t max_frames) {
    this->load_frames(dir, max_frames);
    if (this->frames.size() == 0) {
        throw std::runtime_error("no frames");
    } else {
        fprintf(stderr, "%lu frames loaded.\n", this->frames.size());
    }
}

void FakeCamera::stream_on() {}
void FakeCamera::stream_off() {}

std::pair<void*, uint32_t> FakeCamera::dequeue() {
    auto ret = std::pair<void*, uint32_t>{this->frames[this->index].data(), this->index};
    this->index = index >= (this->frames.size() - 1) ? 0 : index + 1;
    return ret;
}

void FakeCamera::enqueue(const uint32_t) {}

std::pair<uint32_t, uint32_t> FakeCamera::get_size() const {
    return {WIDTH, HEIGHT};
}

std::pair<uint32_t, uint32_t> FakeCamera::get_bytes() const {
    return {SIZEIMAGE, BYTESPERLINE};
}

void FakeCamera::load_frames(const char* dir, const uint32_t max_frames) {
    uint32_t count = 0;
    while (count < max_frames) {
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

} // namespace tofcam
