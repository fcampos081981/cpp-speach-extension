// main.cpp
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <system_error>
#include <cstdlib>
#include <cmath>

#if defined(_WIN32)
  #include <windows.h>
#elif defined(__APPLE__)
  #include <mach-o/dyld.h>
  #include <unistd.h>
  #include <limits.h>
#elif defined(__linux__)
  #include <unistd.h>
  #include <limits.h>
#endif

// stb_image: add stb_image.h to your project folder (https://github.com/nothings/stb)
// This translation unit provides the implementation:
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace fs = std::filesystem;

static std::optional<fs::path> get_executable_path() {
#if defined(_WIN32)
    std::vector<wchar_t> buffer(1024);
    for (;;) {
        DWORD len = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (len == 0) return std::nullopt;
        if (len < buffer.size() - 1) {
            return fs::path(std::wstring(buffer.data(), len));
        }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buf(size);
    if (_NSGetExecutablePath(buf.data(), &size) != 0) return std::nullopt;

    char resolved[PATH_MAX];
    if (realpath(buf.data(), resolved) == nullptr) {
        return fs::path(buf.data());
    }
    return fs::path(resolved);
#elif defined(__linux__)
    std::vector<char> buf(1024);
    for (;;) {
        ssize_t len = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
        if (len < 0) return std::nullopt;
        if (static_cast<size_t>(len) < buf.size() - 1) {
            buf[static_cast<size_t>(len)] = '\0';
            return fs::path(buf.data());
        }
        buf.resize(buf.size() * 2);
    }
#else
    return std::nullopt;
#endif
}

static std::optional<fs::path> executable_dir() {
    if (auto p = get_executable_path()) {
        return p->parent_path();
    }
    return std::nullopt;
}

static std::optional<fs::path> resolve_resource(const std::string& name) {
    std::vector<fs::path> candidates;

    fs::path p{name};
    if (p.is_absolute()) {
        if (fs::exists(p)) return p;
        return std::nullopt;
    }

    candidates.emplace_back(fs::current_path() / name);

    if (auto ed = executable_dir()) {
        candidates.emplace_back(*ed / name);
        candidates.emplace_back(*ed / "resources" / name);
    }

#ifdef PROJECT_SOURCE_DIR
    candidates.emplace_back(fs::path(PROJECT_SOURCE_DIR) / name);
    candidates.emplace_back(fs::path(PROJECT_SOURCE_DIR) / "resources" / name);
#endif
#ifdef PROJECT_BINARY_DIR
    candidates.emplace_back(fs::path(PROJECT_BINARY_DIR) / name);
    candidates.emplace_back(fs::path(PROJECT_BINARY_DIR) / "resources" / name);
#endif

    if (const char* env = std::getenv("RESOURCES_DIR")) {
        candidates.emplace_back(fs::path(env) / name);
    }

    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::exists(c, ec) && !ec) return c;
    }

    std::cerr << "Failed to locate resource '" << name << "'. Searched:\n";
    for (const auto& c : candidates) {
        std::cerr << "  - " << c.string() << "\n";
    }
    return std::nullopt;
}

// Map [0..1] luminance to ASCII gradient (dark -> light)
static char luminance_to_char(float y) {
    static constexpr const char* gradient = "@%#*+=-:. ";
    // You can tweak the gradient for different looks, e.g. "$@B%8&WM#*oahkbdpqwmZ0QLCJUYXzcvunxrjft/\\|()1{}[]?-_+~<>i!lI;:,\"^`'. "
    constexpr int n = 10; // length of gradient above
    int idx = static_cast<int>(y * (n - 1) + 0.5f);
    if (idx < 0) idx = 0;
    if (idx >= n) idx = n - 1;
    return gradient[idx];
}

// Compute perceived luminance from sRGB 8-bit triplet
static inline float srgb_to_luma(unsigned char r, unsigned char g, unsigned char b) {
    // Convert to linear approx, then luma; for simplicity use direct weights on sRGB
    // Better: convert to linear with gamma 2.2 and then apply weights.
    return (0.2126f * (r / 255.0f) + 0.7152f * (g / 255.0f) + 0.0722f * (b / 255.0f));
}

int main(int argc, char** argv) {
    std::cout << "Program path: ";
    if (auto exe = get_executable_path()) {
        std::cout << exe->string() << "\n";
    } else {
        std::cout << "(unknown)\n";
    }

    // Parse args: image path and optional output width
    std::string requested;
    if (argc >= 2) {
        requested = argv[1];
    } else {
        requested = "puppy.png";

        std::cerr << "Usage: " << (argc > 0 ? argv[0] : "ascii") << " <image.(png|jpg|... )> [width]\n";
        std::cerr << "Example: " << (argc > 0 ? argv[0] : "ascii") << " puppy.png 100\n";
        return 1;
    }

    int out_width = 100;
    if (argc >= 3) {
        try {
            out_width = std::max(10, std::stoi(argv[2]));
        } catch (...) {
            std::cerr << "Invalid width; using default 100\n";
        }
    }

    auto imagePath = resolve_resource(requested);
    if (!imagePath) {
        std::cerr << "Could not render ASCII from image (file not found)\n";
        return 1;
    }

    int w = 0, h = 0, comp = 0;
    // Request 3 channels (RGB)
    stbi_uc* pixels = stbi_load(imagePath->string().c_str(), &w, &h, &comp, 3);
    if (!pixels) {
        std::cerr << "Failed to load image: " << imagePath->string() << "\n";
        return 1;
    }

    // Account for character aspect ratio: characters are taller than they are wide.
    // Approximate: a typical console cell is ~2:1 height:width, so use 0.5 factor.
    const float char_aspect = 0.5f;
    int out_height = std::max(1, static_cast<int>(std::round(h * (out_width / static_cast<float>(w)) * char_aspect)));

    // Downsample by averaging source pixels for each output cell (box filter)
    std::vector<char> line(out_width + 1, '\0');

    std::cout << "--- ASCII Art (" << imagePath->filename().string() << ") ---\n";

    for (int oy = 0; oy < out_height; ++oy) {
        // Compute source y-range
        float y0f = (oy / static_cast<float>(out_height)) * h;
        float y1f = ((oy + 1) / static_cast<float>(out_height)) * h;
        int y0 = std::clamp(static_cast<int>(std::floor(y0f)), 0, h - 1);
        int y1 = std::clamp(static_cast<int>(std::ceil(y1f)), 0, h);

        for (int ox = 0; ox < out_width; ++ox) {
            float x0f = (ox / static_cast<float>(out_width)) * w;
            float x1f = ((ox + 1) / static_cast<float>(out_width)) * w;
            int x0 = std::clamp(static_cast<int>(std::floor(x0f)), 0, w - 1);
            int x1 = std::clamp(static_cast<int>(std::ceil(x1f)), 0, w);

            // Average luminance across the block
            double sum = 0.0;
            int count = 0;
            for (int sy = y0; sy < y1; ++sy) {
                const unsigned char* row = pixels + (sy * w * 3);
                for (int sx = x0; sx < x1; ++sx) {
                    const unsigned char* p = row + sx * 3;
                    sum += srgb_to_luma(p[0], p[1], p[2]);
                    ++count;
                }
            }
            float y = count ? static_cast<float>(sum / count) : 0.0f;

            // Optional gamma correction to better use the gradient
            // y = std::pow(y, 0.8f);

            line[ox] = luminance_to_char(1.0f - y); // invert so dark pixels -> dense chars
        }
        line[out_width] = '\0';
        std::cout << line.data() << "\n";
    }

    stbi_image_free(pixels);
    return 0;
}