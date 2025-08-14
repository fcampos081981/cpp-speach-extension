#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <algorithm>


void speakText(const std::string &text);


static char grayscaleToAscii(uint8_t g, bool invert = false) {
    static const std::string gradient = " .'`^\",:;Il!i~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
    const int N = static_cast<int>(gradient.size());

    int idx = static_cast<int>(std::round((g / 255.0) * (N - 1)));
    if (!invert) {
        idx = (N - 1) - idx;
    }
    idx = std::clamp(idx, 0, N - 1);
    return gradient[idx];
}


bool printAsciiFromImage(const std::string &path, int maxWidth = 120, bool invert = false) {
    int w = 0, h = 0, ch = 0;

    unsigned char *data = stbi_load(path.c_str(), &w, &h, &ch, 1);
    if (!data) {
        std::cerr << "Failed to load image: " << path << "\n";
        return false;
    }


    const double charAspect = 2.0;
    const int outWidth = std::max(1, std::min(maxWidth, w));

    const int outHeight = std::max(
        1, static_cast<int>(std::round((h * (outWidth / static_cast<double>(w))) / charAspect)));

    const double sx = w / static_cast<double>(outWidth);
    const double sy = h / static_cast<double>(outHeight);

    for (int oy = 0; oy < outHeight; ++oy) {
        int y0 = static_cast<int>(std::floor(oy * sy));
        int y1 = static_cast<int>(std::floor((oy + 1) * sy));
        y1 = std::max(y1, y0 + 1);
        y1 = std::min(y1, h);

        for (int ox = 0; ox < outWidth; ++ox) {
            int x0 = static_cast<int>(std::floor(ox * sx));
            int x1 = static_cast<int>(std::floor((ox + 1) * sx));
            x1 = std::max(x1, x0 + 1);
            x1 = std::min(x1, w);

            long sum = 0;
            int cnt = 0;
            for (int yy = y0; yy < y1; ++yy) {
                const unsigned char *row = data + yy * w;
                for (int xx = x0; xx < x1; ++xx) {
                    sum += row[xx];
                    ++cnt;
                }
            }
            uint8_t avg = static_cast<uint8_t>(sum / std::max(1, cnt));
            std::cout << grayscaleToAscii(avg, invert);
        }
        std::cout << '\n';
    }

    stbi_image_free(data);
    return true;
}


static std::string threeDigitsToWords(int num) {
    static const std::vector<std::string> below20{
        "", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
        "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen",
        "sixteen", "seventeen", "eighteen", "nineteen"
    };
    static const std::vector<std::string> tens{
        "", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"
    };

    std::string res;
    int hundred = num / 100;
    int rest = num % 100;

    if (hundred) {
        res += below20[hundred] + " hundred";
        if (rest) res += " ";
    }
    if (rest) {
        if (rest < 20) {
            res += below20[rest];
        } else {
            int t = rest / 10;
            int u = rest % 10;
            res += tens[t];
            if (u) res += "-" + below20[u];
        }
    }
    return res;
}

std::string numberToWords(long long num) {
    if (num == 0) return "zero";

    std::string sign;
    if (num < 0) {
        sign = "minus ";
        num = std::llabs(num);
    }

    static const std::vector<std::pair<long long, std::string> > units{
        {1'000'000'000'000LL, "trillion"},
        {1'000'000'000LL, "billion"},
        {1'000'000LL, "million"},
        {1'000LL, "thousand"},
        {1LL, ""}
    };

    std::string result;
    for (auto [value, name]: units) {
        if (num >= value) {
            int chunk = static_cast<int>(num / value);
            num %= value;

            std::string part = threeDigitsToWords(chunk);
            if (!result.empty()) result += " ";
            result += part;
            if (!name.empty()) result += " " + name;
        }
    }

    return sign + result;
}

std::string lettersSeparated(const std::string &word, char sep = ' ', bool uppercase = true) {
    std::string out;
    out.reserve(word.size() * 3);
    bool first = true;
    for (unsigned char ch: word) {
        if (std::isspace(ch)) continue;
        if (!first) out += sep;
        first = false;
        out += static_cast<char>(uppercase ? std::toupper(ch) : ch);
    }
    return out;
}

void speakSpelled(const std::string &word) {
    std::string spelled = lettersSeparated(word, ',', true);

    for (size_t i = 1; i < spelled.size(); ++i) {
        if (spelled[i - 1] == ',') spelled.insert(i++, " ");
    }

    speakText(spelled);
}

static std::string escapeForShellSingleQuotes(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c: s) {
        if (c == '\'') out += "'\"'\"'";
        else out += c;
    }
    return out;
}

static std::string escapeForPowerShellSingleQuotes(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c: s) {
        if (c == '\'') out += "''";
        else out += c;
    }
    return out;
}

void speakText(const std::string &text) {
#if defined(_WIN32)
    std::string t = escapeForPowerShellSingleQuotes(text);
    std::string cmd = "powershell -NoProfile -Command \"$v=New-Object -ComObject SAPI.SpVoice; $null = $v.Speak('" + t +
                      "');\"";
    std::system(cmd.c_str());
#elif defined(__APPLE__)
    std::string t = escapeForShellSingleQuotes(text);
    std::string cmd = "say '" + t + "'";
    std::system(cmd.c_str());
#else
    std::string t = escapeForShellSingleQuotes(text);
    std::string cmd = "espeak '" + t + "'";
    std::system(cmd.c_str());
#endif
}

void speakNumber(long long num) {
    speakText(numberToWords(num));
}

void speakWord(const std::string &text) {
    speakText(text);
}

void drawSquare(int n, char ch = '#', bool filled = true) {
    if (n <= 0) return;
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            if (filled || r == 0 || r == n - 1 || c == 0 || c == n - 1)
                std::cout << ch;
            else
                std::cout << ' ';
        }
        std::cout << '\n';
    }
}

void renderAsciiArt(const std::vector<std::string> &pattern,
                    int scaleX = 1, int scaleY = 1,
                    char on = '#', char off = ' ') {
    if (pattern.empty() || scaleX < 1 || scaleY < 1) return;

    const size_t rows = pattern.size();
    size_t cols = 0;
    for (const auto &row: pattern) cols = std::max(cols, row.size());

    for (size_t r = 0; r < rows; ++r) {
        for (int sy = 0; sy < scaleY; ++sy) {
            for (size_t c = 0; c < cols; ++c) {
                const bool bit = (c < pattern[r].size() && pattern[r][c] != ' ');
                const char ch = bit ? on : off;
                for (int sx = 0; sx < scaleX; ++sx)
                    std::cout << ch;
            }
            std::cout << '\n';
        }
    }
}

void drawCircle(int radius, char ch = 'o', bool filled = false) {
    if (radius <= 0) return;


    const double xScale = 2.0;
    const int height = 2 * radius + 1;
    const int width = static_cast<int>(2 * radius * xScale) + 1;

    const double r = static_cast<double>(radius);
    const double r2 = r * r;
    const double thickness = 0.85;

    for (int y = 0; y < height; ++y) {
        double dy = y - radius;
        for (int x = 0; x < width; ++x) {
            double dx = (x - (width - 1) / 2.0) / xScale;
            double dist2 = dx * dx + dy * dy;

            bool pixel = filled
                             ? (dist2 <= r2 + 0.25)
                             : (std::abs(dist2 - r2) <= thickness);

            std::cout << (pixel ? ch : ' ');
        }
        std::cout << '\n';
    }
}

int main(int argc, char **argv) {
    auto lang = "C++";
    std::cout << "Hello and welcome to " << lang << "!\n";

    for (int i = 1; i <= 1; i++) {
        std::cout << i << " -> " << numberToWords(i) << '\n';
        speakText("Counting a number:");
        speakNumber(i);
    }

    int n = 300;
    int d = 1408;

    std::cout << n << " -> " << numberToWords(n) << '\n';
    std::cout << d << " -> " << numberToWords(d) << '\n';

    speakNumber(n);
    speakNumber(d);

    std::string w = "Morizo";
    std::cout << w << " -> " << lettersSeparated(w, ' ', true) << '\n';
    speakSpelled(w);
    speakWord(w);

    drawSquare(5);
    std::cout << '\n';
    drawSquare(6, '*', false);

    std::cout << "Outline circle (r=8):\n";
    drawCircle(8, '*', false);

    std::cout << "\nFilled circle (r=6):\n";
    drawCircle(6, '#', true);


    std::vector<std::string> heart = {
        "  **   **  ",
        " **** **** ",
        "***********",
        " ********* ",
        "  *******  ",
        "   *****   ",
        "    ***    ",
        "     *     "
    };

    std::cout << "Heart x1:\n";
    renderAsciiArt(heart, 1, 1, '@', ' ');

    std::cout << "\nHeart x2 (scaled):\n";
    renderAsciiArt(heart, 2, 2, '*', ' ');


    std::vector<std::string> smiley = {
        "  *****  ",
        " *     * ",
        "*  * *  *",
        "*       *",
        "*  ---  *",
        " *     * ",
        "  *****  "
    };

    std::cout << "\nSmiley x1:\n";
    renderAsciiArt(smiley, 1, 1, '#', ' ');


    if (argc >= 2) {
        std::string path = argv[1];
        int maxWidth = 120;
        bool invert = false;

        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.rfind("--width=", 0) == 0) {
                try {
                    maxWidth = std::max(1, std::stoi(arg.substr(8)));
                } catch (...) {
                    std::cerr << "Invalid width; using default " << maxWidth << "\n";
                }
            } else if (arg == "--invert") {
                invert = true;
            }
        }

        std::cout << "\n--- ASCII Art (" << path << ") ---\n";
        if (!printAsciiFromImage(path, maxWidth, invert)) {
            std::cerr << "Could not render ASCII from image.\n";
        }
    }

    return 0;
}
