#include "document.h"

using  std::string_literals::operator ""s;

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating)
{}

Document::Document(const Document& doc) = default;

Document::Document(Document&& doc_to_copy)
        : id(std::exchange(doc_to_copy.id, 0))
        , relevance(std::exchange(doc_to_copy.relevance, 0.0))
        , rating(std::exchange(doc_to_copy.rating, 0))
{

}

std::ostream& operator<<(std::ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

Document& Document::operator=(Document&& doc_to_copy) {
    id = std::exchange(doc_to_copy.id, 0);
    relevance = std::exchange(doc_to_copy.relevance, 0.0);
    rating = std::exchange(doc_to_copy.rating, 0);
    return *this;
}
void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
    std::cout << "{ "s
              << "document_id = "s << document_id << ", "s
              << "status = "s << static_cast<int>(status) << ", "s
              << "words ="s;
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}