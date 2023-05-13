#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <cmath>
#include <execution>
#include <future>

#include "string_processing.h"
#include "document.h"
#include "read_input_functions.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ACCURACY = 1e-6;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(StringContainer stop_words);
    explicit SearchServer(const std::string_view stop_words_text);
    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy,  const std::string_view raw_query,
                                           DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    std::map<std::string_view, double> GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    template <typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy policy, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view, int document_id) const;
    template <typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy policy, const std::string_view raw_query, 
                                                                            int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    std::set<std::string, std::less<>> all_words_;
    std::set<std::string_view> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> word_freqs_used_id_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view word) const;
    static bool IsValidWord(const std::string_view word);
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    QueryWord ParseQueryWord(const std::string_view text) const;
    Query ParseQuery(const std::string_view text, bool flag) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindAllDocuments(ExecutionPolicy policy, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(StringContainer stop_words) {
    if (!all_of(stop_words.begin(), stop_words.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
    for (std::string_view word : stop_words) {
        if (!word.empty()) {
            auto it_word = all_words_.insert(std::string(word));
            stop_words_.insert(*it_word.first);
        }
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query, true);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(policy, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < ACCURACY) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query, 
                                                    DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename DocumentPredicate, typename ExecutionPolicy>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy policy, const Query& query, 
                                                    DocumentPredicate document_predicate) const {

    if (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
        return FindAllDocuments(query, document_predicate);
    }

    ConcurrentMap<int, double> document_to_relevance(word_to_document_freqs_.size());
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(),
        [this, &document_to_relevance, document_predicate](std::string_view word) {
            if (word_to_document_freqs_.count(std::string(word)) != 0) {
                const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                for (const auto [document_id, term_freq] : word_to_document_freqs_.at(std::string(word))) {
                    DocumentData document_data;
                    document_data = documents_.at(document_id);
                    if (document_predicate(document_id, document_data.status, document_data.rating)) {
                        document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                    }
                }
            }
        });
    auto document_to_relevance_ = document_to_relevance.BuildOrdinaryMap();
    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(std::string(word)) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(std::string(word))) {
            document_to_relevance_.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance_) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}


