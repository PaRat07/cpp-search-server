#include "search_server.h"

using  std::string_literals::operator ""s;

SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
// from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        documents_words_with_freq_[document_id][word] += inv_word_count;
        word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}



std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(std::string(word))) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(std::string(word))) {
            words.push_back(std::string(word));
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + text + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy, const std::string& text) const {
    Query result;
    auto parse_word = [&] (const std::string_view word) {
        const auto query_word = ParseQueryWord(std::string(word));
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    };
    auto words = SplitIntoWords(text);
    std::for_each(std::execution::par, words.begin(), words.end(), parse_word);
    std::sort(std::execution::par, result.plus_words.begin(), result.plus_words.end());
    const auto last_plus = std::unique(std::execution::par, result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(last_plus, result.plus_words.end());
    std::sort(std::execution::par, result.minus_words.begin(), result.minus_words.end());
    const auto last_mines = std::unique(std::execution::par, result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(last_mines, result.minus_words.end());
    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::sequenced_policy, const std::string& text) const {
    Query result;
    auto parse_word = [&] (const std::string_view word) {
        const auto query_word = ParseQueryWord(std::string(word));
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    };
    auto words = SplitIntoWords(text);
    std::for_each(std::execution::seq, words.begin(), words.end(), parse_word);
    std::sort(std::execution::seq, result.plus_words.begin(), result.plus_words.end());
    const auto last_plus = std::unique(std::execution::seq, result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(last_plus, result.plus_words.end());
    std::sort(std::execution::seq, result.minus_words.begin(), result.minus_words.end());
    const auto last_mines = std::unique(std::execution::seq, result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(last_mines, result.minus_words.end());
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    return (static_cast<bool>(documents_.count(document_id)) ? documents_words_with_freq_.at(document_id) : empty_map_);
}

std::set<int>::const_iterator SearchServer::begin() const { return document_ids_.begin(); }

std::set<int>::const_iterator SearchServer::end() const { return document_ids_.end(); }

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& exec_pol, int document_id) {
    std::vector<const std::string*> data;
    data.reserve(documents_words_with_freq_[document_id].size());
    for (auto& [word, freq] : documents_words_with_freq_[document_id]) {
        data.push_back(&word);
    }
    auto deleter = [&] (const std::string* word) {
        word_to_document_freqs_[*word].erase(document_id);
    };
    std::for_each(std::execution::seq, data.begin(), data.end(), deleter);
    documents_words_with_freq_.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& exec_pol, int document_id) {
    std::vector<const std::string*> data;
    data.reserve(documents_words_with_freq_[document_id].size());
    for (auto& [word, freq] : documents_words_with_freq_[document_id]) {
        data.push_back(&word);
    }
    auto deleter = [&] (const std::string* word) {
        word_to_document_freqs_[*word].erase(document_id);
    };
    std::for_each(std::execution::par, data.begin(), data.end(), deleter);
    documents_words_with_freq_.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy, const std::string& raw_query, int document_id) const {
    if (!document_ids_.count(document_id)) {
        throw std::out_of_range("document with this number doesn't exist");
    }
    const auto query = ParseQuery(std::execution::seq, raw_query);

    std::vector<std::string> matched_words;
    auto is_in_doc = [&] (const std::string& query_word) {
        if (word_to_document_freqs_.count(query_word) == 0) {
            return false;
        }
        if (word_to_document_freqs_.at(query_word).count(document_id)) {
            return true;
        }
        return false;
    };
    if (std::any_of(std::execution::seq, query.minus_words.begin(), query.minus_words.end(), is_in_doc)) {
        matched_words.clear();
        return { matched_words, documents_.at(document_id).status };
    }
    std::copy_if(std::execution::seq, query.plus_words.begin(), query.plus_words.end(),
                 std::back_inserter(matched_words), is_in_doc);


    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy, const std::string& raw_query, int document_id) const {
    if (!document_ids_.count(document_id)) {
        throw std::out_of_range("document with this number doesn't exist");
    }
    const auto query = ParseQuery(std::execution::par, raw_query);

    std::vector<std::string> matched_words;
    auto is_in_doc = [&] (const std::string& query_word) {
        if (word_to_document_freqs_.count(query_word) == 0) {
            return false;
        }
        if (word_to_document_freqs_.at(query_word).count(document_id)) {
            return true;
        }
        return false;
    };
    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), is_in_doc)) {
        matched_words.clear();
        return { matched_words, documents_.at(document_id).status };
    }
    std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
                 std::back_inserter(matched_words), is_in_doc);


    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}