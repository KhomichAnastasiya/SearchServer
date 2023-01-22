#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {

    std::set<int> duplicates_documents;
    std::set<std::set<std::string>> documents;
    std::set<std::string> words_in_documents;

    for (const int document_id : search_server) {

        const std::map<std::string, double>& document_freq = search_server.GetWordFrequencies(document_id);

        for (const auto& [word, freq] : document_freq) {
            words_in_documents.insert(word);

        }

        if (documents.find(words_in_documents) != documents.end()) {
            duplicates_documents.insert(document_id);
        }
        else {
            documents.insert(words_in_documents);
        }
        words_in_documents.clear();
    }


    for (int id : duplicates_documents) {
        std::cout << "Found duplicate document id "s << id << std::endl;
        search_server.RemoveDocument(id);
    }

}