#include "search_server.h"

using  std::string_literals::operator ""s;

SearchServer::SearchServer(const std::string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
// from string container
{
}
SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
// from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document.data());

    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        documents_[document_id].words.push_back(word);
        documents_words_with_freq_[document_id][documents_[document_id].words.back()] += inv_word_count;
        word_to_document_freqs_[documents_[document_id].words.back()][document_id] += inv_word_count;
    }
    documents_[document_id].rating = ComputeAverageRating(ratings);
    documents_[document_id].status = status;
    document_ids_.insert(document_id);
}



std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word.data()) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw std::invalid_argument("Query word "s + text.data() + " is invalid");
    }

    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy, const std::string_view text) const {
    Query result;
    const auto words = SplitIntoWords(text);
    std::vector<QueryWord> query_words(words.size());
    std::transform(std::execution::par, words.begin(), words.end(), query_words.begin(), [&] (const std::string_view word) {
        return ParseQueryWord(word);
    });
    auto return_is_minus = [] (const QueryWord& word) {
        return word.is_minus;
    };
    auto return_is_plus = [] (const QueryWord& word) {
        return !word.is_minus;
    };
    result.plus_words.resize(query_words.size());
    result.plus_words.erase(std::copy_if(std::execution::par, query_words.begin(), query_words.end(),
                                         result.plus_words.begin(), return_is_plus),result.plus_words.end());

    result.minus_words.resize(query_words.size() - result.plus_words.size());
    std::copy_if(std::execution::par, query_words.begin(), query_words.end(), result.minus_words.begin(), return_is_minus);


    return result;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::sequenced_policy, const std::string_view text) const {
    Query result;
    for (const std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word);
            }
            else {
                result.plus_words.push_back(query_word);
            }
        }
    }
    std::sort(result.plus_words.begin(), result.plus_words.end(), std::less<>());
    result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end()), result.plus_words.end());
    std::sort(result.minus_words.begin(), result.minus_words.end(), std::less<>());
    result.minus_words.erase(std::unique(result.minus_words.begin(), result.minus_words.end()), result.minus_words.end());

    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

const std::map<std::string_view, double, std::less<>>& SearchServer::GetWordFrequencies(int document_id) const {
    return (static_cast<bool>(documents_.count(document_id)) ? documents_words_with_freq_.at(document_id) : empty_map_);
}

std::set<int>::const_iterator SearchServer::begin() const { return document_ids_.begin(); }

std::set<int>::const_iterator SearchServer::end() const { return document_ids_.end(); }

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& exec_pol, int document_id) {
    for (auto [word, freq] : documents_words_with_freq_[document_id]) {
        word_to_document_freqs_[word].erase(document_id);
    }
    documents_words_with_freq_.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& exec_pol, int document_id) {
    std::vector<std::string_view> data(documents_words_with_freq_[document_id].size());
    auto first_of_pair = [] (std::pair<std::string_view, double> pair) {
        return pair.first;
    };
    std::transform (std::execution::par, documents_words_with_freq_[document_id].begin(), documents_words_with_freq_[document_id].end(), data.begin(), first_of_pair);
    auto deleter = [&] (std::string_view word) {
        word_to_document_freqs_[word].erase(document_id);
    };
    std::for_each(std::execution::par, data.begin(), data.end(), deleter);
    documents_words_with_freq_.erase(document_id);
    document_ids_.erase(document_id);
    documents_.erase(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy, std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(std::execution::seq, raw_query);

    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word.data()) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word.data()).count(document_id)) {
            matched_words.push_back(*std::find(documents_.at(document_id).words.begin(), documents_.at(document_id).words.end(), word));
        }
    }
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word.data()) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word.data()).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, const int document_id) const {
    const auto query = ParseQuery(std::execution::par, raw_query.data());
    std::vector<std::string_view> doc(documents_.at(document_id).words.size());
    std::copy(std::execution::par, documents_.at(document_id).words.begin(), documents_.at(document_id).words.end(), doc.begin());
    auto is_minus = [&query] (const std::string_view query_word) {
        return std::count(query.minus_words.begin(), query.minus_words.end(), query_word);
    };
    if (std::any_of(std::execution::par, doc.begin(), doc.end(), is_minus)) {
        return { std::vector<std::string_view>(), documents_.at(document_id).status };
    }
    auto is_plus = [&query] (const std::string_view query_word) {
        return std::count(query.plus_words.begin(), query.plus_words.end(), query_word);
    };
    std::vector<std::string_view> matched_words(query.plus_words.size());
    matched_words.erase(std::copy_if(doc.begin(), doc.end(), matched_words.begin(), is_plus), matched_words.end());

    std::sort(matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}