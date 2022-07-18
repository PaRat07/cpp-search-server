#pragma once
#include <set>
#include <string>
#include <vector>
#include <string_view>

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const auto& str : strings) {
        non_empty_strings.insert(std::string(str));
    }
    return non_empty_strings;
}


std::vector<std::string_view> SplitIntoWords(std::string_view text);