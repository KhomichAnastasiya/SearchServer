#pragma once

#include <string>
#include <vector>
#include <string_view>

std::vector<std::string_view> SplitIntoWordsView(std::string_view text);
std::vector<std::string> SplitIntoWords(std::string& text);