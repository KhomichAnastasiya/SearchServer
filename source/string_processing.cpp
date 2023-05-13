#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}


std::vector<std::string_view> SplitIntoWordsView(std::string_view text) {
    std::vector<std::string_view> words;
    while (true) {
        const int64_t space = text.find(' ');
        words.push_back(text.substr(0, space));
        if (space == text.npos) {
            break;
        }
        else {
            text.remove_prefix(space + 1);
        }
    }
    return words;
}