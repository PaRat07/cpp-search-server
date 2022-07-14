#include "document.h"

using namespace std;

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating)
{}

Document::Document(const Document& doc) = default;

Document::Document(Document&& doc_to_copy)
        : id(exchange(doc_to_copy.id, 0))
        , relevance(exchange(doc_to_copy.relevance, 0.0))
        , rating(exchange(doc_to_copy.rating, 0))
{

}

ostream& operator<<(ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

Document& Document::operator=(const Document& doc_to_copy) = default;

Document& Document::operator=(Document&& doc_to_copy) {
    id = exchange(doc_to_copy.id, 0);
    relevance = exchange(doc_to_copy.relevance, 0.0);
    rating = exchange(doc_to_copy.rating, 0);
    return *this;
}
void PrintMatchDocumentResult(int document_id, const vector<string_view>& words, DocumentStatus status) {
    cout << "{ "s
              << "document_id = "s << document_id << ", "s
              << "status = "s << static_cast<int>(status) << ", "s
              << "words ="s;
    for (const string_view word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}