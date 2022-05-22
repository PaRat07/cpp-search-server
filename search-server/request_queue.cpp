#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server)
	:search_server_(search_server)
{}
// сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики


std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
	return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
		return document_status == status;
	});
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
	return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
	return clear_ans;
}

// возможно, здесь вам понадобится что-то ещё
void RequestQueue::ControlSize() {
	while (requests_.size() > min_in_day_) {
		if (requests_.front().empty) {
			--clear_ans;
		}
		requests_.pop_back();
	}
}