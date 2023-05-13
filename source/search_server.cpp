#include "search_server.h"

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(SplitIntoWordsView(stop_words_text)) {}

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWordsView(stop_words_text)) {}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end()const {
    return document_ids_.end();
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (std::string_view word : SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            std::string word_{ word };
            throw std::invalid_argument("Word " + word_ + " is invalid");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

std::map<std::string_view, double> SearchServer::GetWordFrequencies(int document_id) const {
    static std::map<std::string_view, double> empty = {};
    return (word_freqs_used_id_.count(document_id) ? word_freqs_used_id_.at(document_id) : empty);
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, 
                              const std::vector<int>& ratings) {

    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        auto it_word = all_words_.insert(std::string(word));
        word_to_document_freqs_[*it_word.first][document_id] += inv_word_count;
        word_freqs_used_id_[document_id][*it_word.first] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, 
                                                                        int document_id) const {
    if (!document_ids_.count(document_id)) {
        throw std::out_of_range("out_of_range ");
    }
    const Query query = ParseQuery(raw_query, true);

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { {}, documents_.at(document_id).status };
        }
    }
    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

template <typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(ExecutionPolicy policy,
                                                   const std::string_view raw_query, int document_id) const {

    if (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
        return MatchDocument(policy, raw_query, document_id);
    }

    if (document_ids_.count(document_id) == 0) {
        return { {}, {} };
    }
    const Query query = ParseQuery(raw_query, false);

    if (std::any_of(policy,
        query.minus_words.begin(), query.minus_words.end(), [&](std::string_view word) {
            const auto it = word_to_document_freqs_.find(word);
            return it != word_to_document_freqs_.end() && it->second.count(document_id);
        })) {
        return { {}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());
    auto it_copy = std::copy_if(policy,
        query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [&](std::string_view word) {
            const auto it = word_to_document_freqs_.find(word);
            return it != word_to_document_freqs_.end() && it->second.count(document_id);
        });

    sort(matched_words.begin(), it_copy);
    it_copy = unique(matched_words.begin(), it_copy);
    return { {matched_words.begin(), it_copy}, documents_.at(document_id).status };
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        std::string word_{ word };
        throw std::invalid_argument("Query word " + word_ + " is invalid");
    }
    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool flag) const {
    Query result;
    for (const std::string_view word : SplitIntoWordsView(text)) {
        const QueryWord query_word = SearchServer::ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (flag == true) {
        std::sort(result.minus_words.begin(), result.minus_words.end());
        auto last_minus = std::unique(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(last_minus, result.minus_words.end());
        std::sort(result.plus_words.begin(), result.plus_words.end());
        auto last_plus = std::unique(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(last_plus, result.plus_words.end());
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

void SearchServer::RemoveDocument(int document_id) {
    if (!document_ids_.count(document_id)) {
        return;
    }
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    word_freqs_used_id_.erase(document_id);
    for (auto& [word, id_freq] : word_to_document_freqs_) {
        if (id_freq.count(document_id)) {
            id_freq.erase(document_id);
        }
    }
}

template <typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy policy, int document_id) {

    if (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
        return RemoveDocument(document_id);
    }

    if (documents_.count(document_id) == 0) {
        return;
    }
    const auto& word_freqs = word_freqs_used_id_.at(document_id);
    std::vector<std::string> words(word_freqs.size());
    transform(policy, word_freqs.begin(), word_freqs.end(), words.begin(), [](const auto& item) {
        return item.first;
        });
    for_each(policy, words.begin(), words.end(), [this, document_id](std::string word) {
        word_to_document_freqs_.find(word)->second.erase(document_id);
        });

    document_ids_.erase(document_id);
    documents_.erase(document_id);
    word_freqs_used_id_.erase(document_id);
}