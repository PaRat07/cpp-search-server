#pragma once
#include <set>
#include <string>
#include <vector>
#include <string_view>

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const auto& str : strings) {
        non_empty_strings.insert(std::string(str));
    }
    return non_empty_strings;
}


std::vector<std::string> SplitIntoWords(std::string_view text);