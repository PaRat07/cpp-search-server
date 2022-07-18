#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWords(string_view str) {
    vector<string_view> result;
    const int64_t pos_end = str.npos;
    str.remove_prefix(static_cast<int64_t>(str.find_first_not_of(" ")) == pos_end ? str.size() : str.find_first_not_of(" "));


    while (!str.empty()) {
        int64_t space = str.find(' ');
        result.push_back(space == pos_end ? str : str.substr(0, space));
        str.remove_prefix(space == pos_end ? str.size() : space);
        str.remove_prefix(static_cast<int64_t>(str.find_first_not_of(" ")) == pos_end ? str.size() : str.find_first_not_of(" "));
    }
    return result;
}