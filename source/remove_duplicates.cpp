#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {

    std::set<int> duplicates_documents;
    std::set<std::set<std::string_view, std::less<>>> documents;
    std::set<std::string_view, std::less<>> words_in_documents;
    
    for (const int document_id : search_server) {
        const std::map<std::string_view, double> word_freqs = search_server.GetWordFrequencies(document_id);
        transform(word_freqs.begin(), word_freqs.end(), inserter(words_in_documents, words_in_documents.begin()),
            [](const std::pair<std::string_view, double> word) {
                return word.first;
            });
        if (documents.count(words_in_documents) == 0) {
            documents.insert(words_in_documents);
        }
        else {
            duplicates_documents.insert(document_id);
        }
    }

    for (int document_id : duplicates_documents) {
        std::cout << "Found duplicate document id " + document_id << "\n";
        search_server.RemoveDocument(document_id);
    }
}