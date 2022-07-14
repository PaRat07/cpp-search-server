#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <stdexcept>
#include <execution>
#include <utility>
#include <cassert>
#include <deque>

#include "string_processing.h"
#include "read_input_functions.h"
#include "document.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

using  std::string_literals::operator ""s;

class SearchServer {
public:
    SearchServer() = default;

    template <typename StringContainer>
    SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string_view stop_words_text);

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;

    int GetDocumentCount() const;

    std::set<int>::const_iterator begin() const;

    std::set<int>::const_iterator end() const;

    const std::map<std::string_view, double, std::less<>>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::parallel_policy& exec_pol, int document_id);

    void RemoveDocument(const std::execution::sequenced_policy& exec_pol, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy, const std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, const int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::deque<std::string> words;
    };
    std::map<std::string_view, double, std::less<>> empty_map_;
    std::map<int, std::map<std::string_view, double, std::less<>>> documents_words_with_freq_;
    const std::set<std::string> stop_words_;
    std::map<std::string_view, std::map<int, double>, std::less<>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
        operator std::string () const {
            return data.data();
        }
        operator std::string_view () const {
            return data;
        }
    };

    QueryWord ParseQueryWord(const std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::execution::parallel_policy, const std::string_view text) const;

    Query ParseQuery(const std::execution::sequenced_policy, const std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};


template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(std::execution::seq, raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        const double error = 1e-6;
        if (std::abs(lhs.relevance - rhs.relevance) < error) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
    });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const SearchServer::Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.count(word)) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq]: word_to_document_freqs_.at(word)) {
                const auto &document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
    }

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto[document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto[document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}