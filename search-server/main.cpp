#include <numeric>

#include <algorithm>

#include <cmath>

#include <iostream>

#include <map>

#include <set>

#include <string>

#include <vector>

#include "search_server.h"
#include "string_processing.h"


using namespace std;




template<typename Key, typename Value>

ostream& operator<<(ostream& out, const pair<Key, Value>& container) {

    out << container.first << ": " << container.second;

    return out;

}

template<class A,class B>
bool operator !=(vector<A> lhs, vector<B> rhs) {
    return !(equal(lhs.begin(), lhs.end(), rhs.begin()) && lhs.size() == rhs.size());
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

    if (!exp) {

        cout << file << "(" << line << "): " << function << ": ASSERT(" << expr << ") failed." << endl;

        abort();

    }

}



void OutErrorWithHint(bool exp, string expr, int line, string file, string function, string hint) {

    if (!exp) {

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

    const vector<int> ratings = { 1, 2, 3 };

    {

        SearchServer server;

        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto found_docs = server.FindTopDocuments("in"s);

        ASSERT_EQUAL(found_docs.size(), 1u);

        const Document& doc0 = found_docs[0];

        ASSERT_EQUAL(doc0.id, doc_id);

    }



    {

        SearchServer server("in the"s);

        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);

    }

}



// Тест проверяет, что поисковая система правильно сортирует документы 

void TestSorting() {

    SearchServer search_server("и в на"s);



    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });

    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    auto docs = search_server.FindTopDocuments("пушистый ухоженный кот"s);



    ASSERT_EQUAL(docs[0].id, 1);

    ASSERT_EQUAL(docs[1].id, 0);

    ASSERT_EQUAL(docs[2].id, 2);

}



// Тест проверяет, что поисковая система исключает минус-слова при добавлении документов 

void TestMinesWords() {

    SearchServer search_server("и в на"s);



    search_server.AddDocument(0, "гот кот пес"s, DocumentStatus::ACTUAL, { 8, -3 });

    search_server.AddDocument(1, "кот гот вот"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

    search_server.AddDocument(2, "кот пес гот"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

    search_server.AddDocument(3, "кот гот"s, DocumentStatus::ACTUAL, { 9 });



    ASSERT_EQUAL(search_server.FindTopDocuments("кот").size(), 4u);

    ASSERT_EQUAL(search_server.FindTopDocuments("кот -пес").size(), 2u);

    ASSERT_EQUAL(search_server.FindTopDocuments("кот -вот").size(), 3u);

    ASSERT(search_server.FindTopDocuments("гот -кот").empty());

}



// Тест проверяет, что поисковая система правильно рассчитывает рейтинг 

void TestCalculatingRating() {

    SearchServer search_server("и в на"s);



    search_server.AddDocument(0, "кот"s, DocumentStatus::ACTUAL, { 8, -3 });

    search_server.AddDocument(1, "кот"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

    search_server.AddDocument(2, "кот"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

    search_server.AddDocument(3, "кот"s, DocumentStatus::ACTUAL, { 9 });

    auto tes_ans = search_server.FindTopDocuments("кот");



    ASSERT_EQUAL(tes_ans[0].rating, 9);

    ASSERT_EQUAL(tes_ans[1].rating, (7 + 2 + 7) / 3);

    ASSERT_EQUAL(tes_ans[2].rating, (8 - 3) / 2);

    ASSERT_EQUAL(tes_ans[3].rating, (5 - 12 + 2 + 1) / 4);

}



void TestMatching() {

    SearchServer search_server("и в на"s);



    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });

    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, { 7, 2, 7 });

    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::IRRELEVANT, { 5, -12, 2, 1 });



    DocumentStatus status;

    vector<string_view> words;

    tie(words, status) = search_server.MatchDocument("белый кот и модный ошейник", 0);

    ASSERT(status == DocumentStatus::ACTUAL);



    vector<string> correct_ans = { "белый", "кот", "модный", "ошейник" };

    ASSERT_EQUAL(words, correct_ans);



    tie(words, status) = search_server.MatchDocument("хвост", 1);

    ASSERT(status == DocumentStatus::BANNED);

    ASSERT_EQUAL(words[0], "хвост");

    ASSERT_EQUAL(words.size(), 1u);



    tie(words, status) = search_server.MatchDocument("", 2);

    ASSERT(status == DocumentStatus::IRRELEVANT);

    ASSERT(words.empty());

}



// Тест проверяет, что поисковая система добавляет документы 

void TestSize() {

    SearchServer search_server("и в на"s);



    search_server.AddDocument(1, "кот"s, DocumentStatus::ACTUAL, { 9 });

    ASSERT_EQUAL(search_server.GetDocumentCount(), 1);



    search_server.AddDocument(2, "кот"s, DocumentStatus::ACTUAL, { 9 });

    ASSERT_EQUAL(search_server.GetDocumentCount(), 2);



    search_server.AddDocument(3, "кот"s, DocumentStatus::ACTUAL, { 9 });

    ASSERT_EQUAL(search_server.GetDocumentCount(), 3);

}



// Тест проверяет, что поисковая система может искать докупенты по предикатам 

void TestSearchingByPredicates() {

    SearchServer search_server("и в на"s);



    search_server.AddDocument(0, "кот"s, DocumentStatus::ACTUAL, { 8, -3 });

    search_server.AddDocument(1, "кот"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });

    search_server.AddDocument(2, "кот"s, DocumentStatus::REMOVED, { 5, -12, 2, 1 });

    search_server.AddDocument(3, "кот"s, DocumentStatus::BANNED, { 9 });

    auto func = [](int document_id, DocumentStatus status, int rating) {

        return status != DocumentStatus::ACTUAL and document_id != 2 and rating != 9;

    };

    auto ans = search_server.FindTopDocuments("кот", func);



    ASSERT_EQUAL(ans.size(), 1u);

    ASSERT_EQUAL(ans[0].id, 1);

    ASSERT_EQUAL(ans[0].relevance, 0);

    ASSERT_EQUAL(ans[0].rating, 5);

}



// Тест проверяет, что поисковая система правильно ищет документы по статусу 

void TestSearchingByStatus() {

    SearchServer search_server("и в на"s);



    search_server.AddDocument(0, "кот"s, DocumentStatus::ACTUAL, { 8, -3 });

    search_server.AddDocument(1, "кот"s, DocumentStatus::REMOVED, { 7, 2, 7 });

    search_server.AddDocument(2, "кот"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

    search_server.AddDocument(3, "кот"s, DocumentStatus::BANNED, { 9 });

    auto ans = search_server.FindTopDocuments("кот", DocumentStatus::ACTUAL);



    ASSERT_EQUAL(ans.size(), 2u);

    ASSERT_EQUAL(ans[0].id, 0);

    ASSERT_EQUAL(ans[1].id, 2);

}



// Тест проверяет, что поисковая система правильно рассчитывает релевантность 

void TestCalculatingRelevance() {

    SearchServer search_server("и в на"s);



    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });

    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });

    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    auto ans = search_server.FindTopDocuments("пушистый ухоженный кот"s);



    ASSERT_EQUAL(ans[0].relevance, log(search_server.GetDocumentCount() / 1.0) * (2.0 / 4.0) + log(4.0 / 2.0) * (1.0 / 4.0));

    ASSERT_EQUAL(ans[1].relevance, log(search_server.GetDocumentCount() / 2.0) * (1.0 / 4.0));

    ASSERT_EQUAL(ans[2].relevance, log(search_server.GetDocumentCount() / 2.0) * (1.0 / 4.0));

}



template <typename Func>

void RunTestImpl(Func tester, string func_name) {

    tester();

    cerr << func_name << " OK" << endl;

}



// Функция TestSearchServer является точкой входа для запуска тестов 

void TestSearchServer() {

//    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);

    RUN_TEST(TestCalculatingRelevance);

    RUN_TEST(TestSize);

    RUN_TEST(TestSearchingByStatus);

    RUN_TEST(TestSearchingByPredicates);

    RUN_TEST(TestMatching);

    RUN_TEST(TestMinesWords);

    RUN_TEST(TestCalculatingRating);

    RUN_TEST(TestSorting);

}

// --------- Окончание модульных тестов поисковой системы ----------- 



int main() {
    setlocale(LC_ALL, "rus");
    TestSearchServer();

    // Если вы видите эту строку, значит все тесты прошли успешно

    cout << "Search server testing finished"s << endl;

}