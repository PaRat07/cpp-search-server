#pragma once
#include <iostream>
#include <string_view>

#include "search_server.h"
#include "document.h"

void AddDocument(SearchServer& search_server, int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);
void MatchDocuments(const SearchServer& search_server, const std::string_view query);
void FindTopDocuments(const SearchServer& search_server, const std::string_view raw_query);