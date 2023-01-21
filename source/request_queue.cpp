#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) :search_server_(search_server)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> documents_ = search_server_.FindTopDocuments(raw_query, status);
    AddEmptyRequest(documents_);
    requests_.push_back({ documents_, raw_query });
    DeleteEmptyRequest();
    return documents_;
}
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> documents_ = search_server_.FindTopDocuments(raw_query);
    AddEmptyRequest(documents_);
    requests_.push_back({ documents_, raw_query });
    DeleteEmptyRequest();
    return documents_;

}
void RequestQueue::AddEmptyRequest(std::vector<Document>& documents_) {
    if (documents_.size() == 0) {
        ++empty_requests_;
    }
}

void RequestQueue::DeleteEmptyRequest() {
    if (requests_.size() > min_in_day_) {
        --empty_requests_;
        requests_.pop_front();
    }
}