#pragma once

#include <vector>
#include <string>
#include <stack>

#include "search_server.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        auto ans = search_server_.FindTopDocuments(raw_query, document_predicate);
        if (ans.empty()) {
            ++clear_ans;
        }

        requests_.push_back({ ans, ans.empty() });
        UpdateStats();
        return ans;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
private:
    struct QueryResult {
        std::vector<Document> data;
        bool empty;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int clear_ans = 0;
    const SearchServer& search_server_;
    void UpdateStats();
};