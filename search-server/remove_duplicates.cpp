#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> ids(search_server.begin(), search_server.end());
    std::map<std::set<std::string>, std::set<int>> docs;
    std::set<int> deleted_docs;

    for (const int doc_id : ids) {
        std::set<std::string> words_in_doc;
        for (const auto& [word, _] : search_server.GetWordFrequencies(doc_id)) {
            words_in_doc.insert(word);
        }
        docs[words_in_doc].insert(doc_id);
    }

    for (const auto& [_, docs_id] : docs) {
        bool is_first = true;
        for (const int doc : docs_id) {
            if (is_first) {
                is_first = false;
                continue;
            }
            deleted_docs.insert(doc);
            search_server.RemoveDocument(doc);
        }
    }

    for (const int i : deleted_docs) {
        std::cout << "Found duplicate document id " << i << std::endl;
    }
}