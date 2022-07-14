#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <utility>

struct Document {
    Document();
    Document(const Document& doc);
    Document(Document&& doc_to_copy);

    Document(int id, double relevance, int rating);
    Document& operator=(Document&& doc_to_copy);
    Document& operator=(const Document& doc_to_copy);
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

std::ostream& operator<<(std::ostream& out, const Document& document);

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status);