#include <iostream>

#include "search_server.h"
#include "request_queue.h"
#include "paginator.h"
#include "document.h"
#include "tests.h"
#include "log_duration.h"

using namespace std;

template <typename Iterator>
ostream& operator<<(ostream& out, const IteratorRange<Iterator> iterator_type) {
    for (auto it = iterator_type.begin(); it != iterator_type.end(); ++it)
    {
        out << *it;
    }
    return out;
}

ostream& operator<<(ostream& out, const Document& document) {
    out << "{ document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;

    return out;
}

void Operation_time()
{
    SearchServer search_server;
    //search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    //server.MatchDocument(search_server, "пушистый -пёс"s);
    {
        LOG_DURATION_STREAM("Operation time:", cout);
        search_server.MatchDocument("пушистый -пёс"s, 0);
    }
    {
        LOG_DURATION_STREAM("Operation time:", cout);
        search_server.FindTopDocuments("пушистый -кот"s);
    }

}

int main() {
    //TestSearchServer();
    Operation_time();
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    system("pause");
    return 0;
}
