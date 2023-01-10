#pragma once

#include <deque>
#include <vector>

#include "search_server.h"
#include "document.h"

using namespace std::string_literals;

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const {
        return empty_requests_;
    }
private:
    struct QueryResult {
        std::vector<Document> documents_;
        std::string query_;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;

    const SearchServer& search_server_;

    void AddEmptyRequest(std::vector<Document>& documents_);
    void DeleteEmptyRequest();

    int empty_requests_ = 0;
    std::string empty_query_ = "empty request"s;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> documents_ = search_server_.FindTopDocuments(raw_query, document_predicate);
    AddEmptyRequest(documents_);
    requests_.push_back({ documents_, raw_query });
    DeleteEmptyRequest();
    return documents_;
}
