#include <execution>
#include <algorithm>
#include <utility>
#include <numeric>
#include <vector>
#include <list>

#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer& search_server,
        const std::vector<std::string>& queries);

std::list<Document> ProcessQueriesJoined(
        const SearchServer& search_server,
        const std::vector<std::string>& queries);


template<typename ExecutionPolicy>
std::list<Document> ProcessQueriesJoined(
        ExecutionPolicy exec_pol,
        const SearchServer& search_server,
        const std::vector<std::string>& queries) {
    auto searcher = [&search_server](const std::string& query) {
        auto result(search_server.FindTopDocuments(query));
        return std::list<Document>(result.begin(), result.end());
    };
    auto merger = [](std::list<Document> lhs, std::list<Document> rhs) {
        for (auto it = std::make_move_iterator(rhs.begin()); it != std::make_move_iterator(rhs.end()); ++it) {
            lhs.push_back(std::move(*it));
        }
        return lhs;
    };
    return std::transform_reduce(exec_pol, queries.begin(), queries.end(),
                                 std::list<Document>(), merger, searcher);
}