#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cctype>


void speakText(const std::string& text);

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

    static const std::vector<std::pair<long long, std::string>> units{
        {1'000'000'000'000LL, "trillion"},
        {1'000'000'000LL,     "billion"},
        {1'000'000LL,         "million"},
        {1'000LL,             "thousand"},
        {1LL,                 ""}
    };

    std::string result;
    for (auto [value, name] : units) {
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

std::string lettersSeparated(const std::string& word, char sep = ' ', bool uppercase = true) {
    std::string out;
    out.reserve(word.size() * 3);
    bool first = true;
    for (unsigned char ch : word) {
        if (std::isspace(ch)) continue;
        if (!first) out += sep;
        first = false;
        out += static_cast<char>(uppercase ? std::toupper(ch) : ch);
    }
    return out;
}

void speakSpelled(const std::string& word) {
    std::string spelled = lettersSeparated(word, ',', true);

    for (size_t i = 1; i < spelled.size(); ++i) {
        if (spelled[i - 1] == ',') spelled.insert(i++, " ");
    }

    speakText(spelled);
}

static std::string escapeForShellSingleQuotes(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\'') out += "'\"'\"'";
        else out += c;
    }
    return out;
}

static std::string escapeForPowerShellSingleQuotes(const std::string& s) {
    // In PowerShell single-quoted strings, escape ' by doubling it
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\'') out += "''";
        else out += c;
    }
    return out;
}

void speakText(const std::string& text) {
#if defined(_WIN32)
    std::string t = escapeForPowerShellSingleQuotes(text);
    std::string cmd = "powershell -NoProfile -Command \"$v=New-Object -ComObject SAPI.SpVoice; $null = $v.Speak('" + t + "');\"";
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

void speakWord(const std::string& text) {
    speakText(text);
}

int main() {

    auto lang = "C++";
    std::cout << "Hello and welcome to " << lang << "!\n";

    for (int i = 1; i <= 1; i++) {
        std::cout << i << " -> " << numberToWords(i) << '\n';
        speakNumber(i);
    }

    int n = 300;
    int d = 1408;

    std::cout << n << " -> " << numberToWords(n) << '\n';
    std::cout << d << " -> " << numberToWords(d) << '\n';


    std::string w = "Morizo";
    std::cout << w << " -> " << lettersSeparated(w, ' ', true) << '\n';
    speakSpelled(w);

    speakNumber(n);
    speakNumber(d);

    return 0;
}