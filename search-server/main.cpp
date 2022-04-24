/* Подставьте вашу реализацию класса SearchServer сюда */
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
template<typename Key, typename Value>
ostream& operator<<(ostream& out, const pair<Key, Value>& container) {
    out << container.first << ": " << container.second;
    return out;
}

template <typename Container>
void Print(ostream& out, const Container& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (!is_first) {
            out << ", "s;
        }
        is_first = false;
        out << element;
    }
}

template<typename Key, typename Value>
ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}

template<typename elem>
ostream& operator<<(ostream& out, const vector<elem>& container) {
    out << "[";
    Print(out, container);
    out << "]";
    return out;
}

template<typename elem>
ostream& operator<<(ostream& out, const set<elem>& container) {
    out << "{";
    Print(out, container);
    out << "}";
    return out;
}

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                                   ComputeAverageRating(ratings),
                                   status
                           });
    }

    template<typename Function>
    vector<Document> FindTopDocuments(const string& raw_query, Function func) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, func);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, const DocumentStatus& status = DocumentStatus::ACTUAL) const {
        auto func = [status]([[maybe_unused]]const int document_id, const DocumentStatus document_status, [[maybe_unused]]const int rating) {return document_status == status;};
        return FindTopDocuments(raw_query, func);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
                text,
                is_minus,
                IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename Function>
    vector<Document> FindAllDocuments(const Query& query, Function func) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (func(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                                                document_id,
                                                relevance,
                                                documents_.at(document_id).rating
                                        });
        }
        return matched_documents;
    }
};

/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/
void AssertEqualImpl(const double & t, const double & u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    const double error = 1e-6;
    if (abs(t - u) > error) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}
void OutError(bool exp, string expr, int line, string file, string function) {
    if (!exp){
        cout << file << "(" << line << "): " << function << ": ASSERT(" << expr << ") failed." << endl;
        abort();
    }
}
void OutErrorWithHint(bool exp, string expr, int line, string file, string function, string hint) {
    if (!exp){
        cout << file << "(" << line << "): " << function << ": ASSERT(" << expr << ") failed. Hint: " << hint << endl;
        abort();
    }
}
template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) OutError(expr, #expr, __LINE__, __FILE__, __FUNCTION__ )
#define ASSERT_HINT(expr, hint) OutErrorWithHint(expr, #expr, __LINE__, __FILE__, __FUNCTION__, hint)
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
#define RUN_TEST(func) RunTestImpl(func, #func)

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/


// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void SortTest() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    auto docs = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(docs[0].id, 1);
    ASSERT_EQUAL(docs[1].id, 0);
    ASSERT_EQUAL(docs[2].id, 2);
}

void MinesTest() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "гот кот пес"s,DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "кот гот вот"s,DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "кот пес гот"s,DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "кот гот"s,DocumentStatus::ACTUAL, {9});

    ASSERT_EQUAL(search_server.FindTopDocuments("кот -пес").size(), 2);
    ASSERT_EQUAL(search_server.FindTopDocuments("кот -вот").size(), 3);
    ASSERT(search_server.FindTopDocuments("гот -кот").empty());
}

void RatingTest() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "кот"s,DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "кот"s,DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "кот"s,DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "кот"s,DocumentStatus::ACTUAL, {9});

    auto tes_ans = search_server.FindTopDocuments("кот");
    ASSERT_EQUAL(tes_ans[0].rating, 9);
    ASSERT_EQUAL(tes_ans[1].rating, 5);
    ASSERT_EQUAL(tes_ans[2].rating, 2);
    ASSERT_EQUAL(tes_ans[3].rating, -1);
}

void MatchTest() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::BANNED, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, {5, -12, 2, 1});
    DocumentStatus status;
    vector<string> words;
    tie(words, status) = search_server.MatchDocument("белый кот и модный ошейник", 0);
    ASSERT(status == DocumentStatus::ACTUAL);
    vector<string> correct_ans = {"белый", "кот", "модный", "ошейник"};
    ASSERT_EQUAL(words, correct_ans);
    tie(words, status) = search_server.MatchDocument("хвост", 1);
    ASSERT(status == DocumentStatus::BANNED);
    ASSERT_EQUAL(words[0], "хвост");
    ASSERT_EQUAL(words.size(), 1);
    tie(words, status) = search_server.MatchDocument("", 2);
    ASSERT(status == DocumentStatus::IRRELEVANT);
    ASSERT(words.empty());
}

void SizeCheck() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(1, "кот"s, DocumentStatus::ACTUAL, {9});
    ASSERT_EQUAL(search_server.FindTopDocuments("кот").size(), 1);
    search_server.AddDocument(2, "кот"s, DocumentStatus::ACTUAL, {9});
    ASSERT_EQUAL(search_server.FindTopDocuments("кот").size(), 2);
    search_server.AddDocument(3, "кот"s, DocumentStatus::ACTUAL, {9});
    ASSERT_EQUAL(search_server.FindTopDocuments("кот").size(), 3);
    search_server.AddDocument(4, "кот"s, DocumentStatus::ACTUAL, {9});
    ASSERT_EQUAL(search_server.FindTopDocuments("кот").size(), 4);
    search_server.AddDocument(5, "кот"s, DocumentStatus::ACTUAL, {9});
    ASSERT_EQUAL(search_server.FindTopDocuments("кот").size(), 5);
    search_server.AddDocument(6, "кот"s, DocumentStatus::ACTUAL, {9});
    search_server.AddDocument(7, "кот"s, DocumentStatus::ACTUAL, {9});
    search_server.AddDocument(8, "кот"s, DocumentStatus::ACTUAL, {9});
    search_server.AddDocument(9, "кот"s, DocumentStatus::ACTUAL, {9});
    search_server.AddDocument(10, "кот"s, DocumentStatus::ACTUAL, {9});
    search_server.AddDocument(0, "кот"s, DocumentStatus::ACTUAL, {9});
    ASSERT_EQUAL(search_server.FindTopDocuments("кот").size(), 5);
}

void UserPred() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "кот"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "кот"s,       DocumentStatus::IRRELEVANT, {7, 2, 7});
    search_server.AddDocument(2, "кот"s, DocumentStatus::REMOVED, {5, -12, 2, 1});
    search_server.AddDocument(3, "кот"s,         DocumentStatus::BANNED, {9});
    auto func = [] (int document_id, DocumentStatus status, int rating) {
        return status != DocumentStatus::ACTUAL and document_id != 2 and rating != 9;
    };

    auto ans = search_server.FindTopDocuments("кот", func);
    ASSERT_EQUAL(ans.size(), 1);
    ASSERT_EQUAL(ans[0].id, 1);
    ASSERT_EQUAL(ans[0].relevance, 0);
    ASSERT_EQUAL(ans[0].rating, 5);
}

void GeneralCase() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    vector<Document> ans = {{1, 0.866434, 5}, {0, 0.173287, 2}, {2, 0.173287, -1}};
    auto docs = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(docs.size(), ans.size());
    for (size_t i = 0; i < docs.size(); ++ i) {
        ASSERT_EQUAL(docs[i].rating, ans[i].rating);
        ASSERT_EQUAL(docs[i].relevance, ans[i].relevance);
        ASSERT_EQUAL(docs[i].id, ans[i].id);
    }
    ans = {{3,  0.231049, 9}};
    docs = search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(docs.size(), ans.size());
    for (size_t i = 0; i < docs.size(); ++ i) {
        ASSERT_EQUAL(docs[i].rating, ans[i].rating);
        ASSERT_EQUAL(docs[i].relevance, ans[i].relevance);
        ASSERT_EQUAL(docs[i].id, ans[i].id);
    }
    ans = {{0, 0.173287, 2}, {2, 0.173287, -1}};
    auto func = [](int document_id, [[maybe_unused]] DocumentStatus status, [[maybe_unused]] int rating) { return document_id % 2 == 0; };
    docs = search_server.FindTopDocuments("пушистый ухоженный кот"s, func);
    ASSERT_EQUAL(docs.size(), ans.size());
    for (size_t i = 0; i < docs.size(); ++ i) {
        ASSERT_EQUAL(docs[i].rating, ans[i].rating);
        ASSERT_EQUAL(docs[i].relevance, ans[i].relevance);
        ASSERT_EQUAL(docs[i].id, ans[i].id);
    }
}

void SearchByStatus() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "кот"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "кот"s,       DocumentStatus::REMOVED, {7, 2, 7});
    search_server.AddDocument(2, "кот"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "кот"s,         DocumentStatus::BANNED, {9});

    auto ans = search_server.FindTopDocuments("кот", DocumentStatus::ACTUAL);
    for (auto elem : ans) {
        ASSERT(elem.id == 0 or elem.id == 2);
    }
}

void TestCommutingRelevance () {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    auto ans = search_server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(ans[0].relevance, 0.866434);
    ASSERT_EQUAL(ans[1].relevance, 0.173287);
    ASSERT_EQUAL(ans[2].relevance, 0.173287);
}

/*
Разместите код остальных тестов здесь
*/

// Функция TestSearchServer является точкой входа для запуска тестов

template <typename Func>
void RunTestImpl(Func tester, string func_name) {
    /* Напишите недостающий код */
    tester();
    cerr << func_name << " OK" << endl;
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestCommutingRelevance);
    RUN_TEST(SearchByStatus);
    RUN_TEST(UserPred);
    RUN_TEST(SortTest);
    RUN_TEST(MinesTest);
    RUN_TEST(RatingTest);
    RUN_TEST(MatchTest);
    RUN_TEST(SizeCheck);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(GeneralCase);
// Не забудьте вызывать остальные тесты здесь
}
// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}