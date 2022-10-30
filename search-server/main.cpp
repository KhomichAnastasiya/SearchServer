#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <utility>
#include <cassert>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double ACCURACY = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}


class SearchServer {
public:

    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    }


    template<typename Predicate>

    vector<Document> FindTopDocuments(const string& raw_query, Predicate predicate,
        DocumentStatus status = DocumentStatus::ACTUAL) const {

        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < ACCURACY) {
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        auto matched_documents = FindTopDocuments(raw_query,
            [status](int document_id, DocumentStatus _status, int rating)
            {
                return _status == status;
            });

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        auto matched_documents = FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
        return matched_documents;
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }


    template<typename Predicate>

    vector<Document> FindAllDocuments(const Query& query, Predicate predicate,
        DocumentStatus status = DocumentStatus::ACTUAL) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

template <typename TFunc>
void RunTestImpl(TFunc func_name_test, const string& func_name) {
    cerr << func_name << " OK"s << endl;
    func_name_test();
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define RUN_TEST(func) RunTestImpl(func, #func)

// -------- Начало модульных тестов поисковой системы ----------

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

void TestAddDocuments()
{
    SearchServer server;
    ASSERT_EQUAL(server.GetDocumentCount(), 0);
    const int doc_id1 = 42;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    ASSERT_EQUAL(server.GetDocumentCount(), 1);
    const int doc_id2 = 43;
    const string content2 = "black dog"s;
    const vector<int> ratings2 = { 1, 2 };
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    ASSERT_EQUAL(server.GetDocumentCount(), 2);
}

void TestExcludeMinusWordsFromAddedDocumentContent()
{
    const int doc_id1 = 42;
    const string content1 = "fluffy black dog"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "A cat with green eyes"s;
    const vector<int> ratings2 = { 1, 2, 3, 4 };

    const int doc_id3 = 44;
    const string content3 = "fluffy rabbit"s;
    const vector<int> ratings3 = { 1, 2 };

    const string query = "white cat -fluffy"s;

    SearchServer server;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);

    ASSERT_EQUAL(server.GetDocumentCount(), 3);
    auto matched_documents = server.FindTopDocuments(query);
    ASSERT_EQUAL(matched_documents.size(), 1);
}

void TestMatchDocuments()
{
    const int doc_id1 = 42;
    const string content1 = "fluffy black dog"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc_id2 = 43;
    const string content2 = "A cat with green eyes"s;
    const vector<int> ratings2 = { 1, 2, 3, 4 };

    const int doc_id3 = 44;
    const string content3 = "black cat"s;
    const vector<int> ratings3 = { 2, 1 };

    SearchServer server;
    Document doc;

    const string query = "white cat -fluffy"s;

    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    auto matched_documents1 = server.FindTopDocuments(query);
    ASSERT_EQUAL(matched_documents1.size(), 0);

    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    auto matched_documents2 = server.FindTopDocuments(query);
    vector<Document> expexted_matched_documents2 = { {doc.id = 43, doc.relevance = 0.0, doc.rating = 2} };

    ASSERT_EQUAL(matched_documents2.size(), 1);
    ASSERT_EQUAL(matched_documents2[0].id, expexted_matched_documents2[0].id);
    ASSERT_EQUAL(matched_documents2[0].rating, expexted_matched_documents2[0].rating);

    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
    auto matched_documents3 = server.FindTopDocuments(query);
    vector<Document> expexted_matched_documents3 = { {doc.id = 44, doc.relevance = 0.0, doc.rating = 1},
                                                     {doc.id = 43, doc.relevance = 0.0, doc.rating = 2} };
    ASSERT_EQUAL(matched_documents3.size(), 2);
    ASSERT_EQUAL(matched_documents3[0].id, expexted_matched_documents3[0].id);
    ASSERT_EQUAL(matched_documents3[1].id, expexted_matched_documents3[1].id);
    ASSERT_EQUAL(matched_documents3[0].rating, expexted_matched_documents3[0].rating);
    ASSERT_EQUAL(matched_documents3[1].rating, expexted_matched_documents3[1].rating);

}

void TestSortDocumentsByRelevance()
{
    const int doc_id1 = 43;
    const string content1 = "A cat with green eyes"s;
    const vector<int> ratings1 = { 1, 2, 3, 4 };

    const int doc_id2 = 44;
    const string content2 = "white cat"s;
    const vector<int> ratings2 = { 1, 2 };

    const int doc_id3 = 45;
    const string content3 = "white cow"s;
    const vector<int> ratings3 = { 1, 3 };

    const int doc_id4 = 46;
    const string content4 = "white parrot with green spots"s;
    const vector<int> ratings4 = { 3, 1, 3, 4 };

    const string query = "white cat"s;

    SearchServer server;

    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
    server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);

    vector<Document> sorted_vector_pets = server.FindTopDocuments(query, [](int document_id,
        DocumentStatus status, int rating) { return document_id > 0; });

    ASSERT_EQUAL(sorted_vector_pets.size(), 4);
    ASSERT_EQUAL(sorted_vector_pets[0].id, doc_id2);
    ASSERT_EQUAL(sorted_vector_pets[1].id, doc_id3);
    ASSERT_EQUAL(sorted_vector_pets[2].id, doc_id1);
    ASSERT_EQUAL(sorted_vector_pets[3].id, doc_id4);
}

void TestCalculatingRatingDocuments()
{
    set<int> average;

    const int doc_id1 = 42;
    const string content1 = "fluffy black cat"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    int average1 = (accumulate(ratings1.begin(), ratings1.end(), 0)) / ratings1.size();

    const int doc_id2 = 43;
    const string content2 = "A cat with green eyes"s;
    const vector<int> ratings2 = { 2, 5, 3, 6 };
    int average2 = (accumulate(ratings2.begin(), ratings2.end(), 0)) / ratings2.size();

    const int doc_id3 = 44;
    const string content3 = "white cat"s;
    const vector<int> ratings3 = { 5, 5 };
    int average3 = (accumulate(ratings3.begin(), ratings3.end(), 0)) / ratings3.size();

    const string query = "white cat"s;

    SearchServer server;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);

    auto matched_documents = server.FindTopDocuments(query);

    ASSERT_EQUAL(matched_documents.size(), 3);
    ASSERT_EQUAL(matched_documents[0].rating, average3);
    ASSERT_EQUAL(matched_documents[1].rating, average2);
    ASSERT_EQUAL(matched_documents[2].rating, average1);

}

void TestFilteringSearchResults()
{
    const int doc_id1 = 9;
    const string content1 = "fluffy black cat"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc_id2 = 7;
    const string content2 = "A cat with green eyes"s;
    const vector<int> ratings2 = { 2, 5, 3, 6 };

    const int doc_id3 = 3;
    const string content3 = "white cat"s;
    const vector<int> ratings3 = { 5, 5 };

    const string query = "white cat"s;

    SearchServer server;

    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);

    vector<Document> matched_documents = server.FindTopDocuments(query, [](int document_id,
        DocumentStatus status, int rating) { return document_id % 3 == 0; });

    ASSERT_EQUAL(matched_documents.size(), 2);
    ASSERT_EQUAL(matched_documents[0].id, 3);
    ASSERT_EQUAL(matched_documents[1].id, 9);
}

void TestSearchDocementsByStatus()
{
    const int doc_id1 = 9;
    const string content1 = "fluffy black cat"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc_id2 = 10;
    const string content2 = "green parrot"s;
    const vector<int> ratings2 = { 1, 2 };

    const int doc_id3 = 11;
    const string content3 = "orange parrot"s;
    const vector<int> ratings3 = { 1, 1 };

    const int doc_id4 = 12;
    const string content4 = "A cat with green eyes"s;
    const vector<int> ratings4 = { 2, 5, 3, 6 };

    const int doc_id5 = 13;
    const string content5 = "white cat"s;
    const vector<int> ratings5 = { 5, 5 };

    const string query = "white parrot"s;

    SearchServer server;

    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
    server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);
    server.AddDocument(doc_id5, content5, DocumentStatus::BANNED, ratings5);

    vector<Document> matched_documents = server.FindTopDocuments(query, DocumentStatus::BANNED);
    ASSERT_EQUAL(matched_documents.size(), 2);
    ASSERT_EQUAL(matched_documents[0].id, doc_id5);
    ASSERT_EQUAL(matched_documents[1].id, doc_id2);
}

int CountMatchWordInAllDocuments(vector<string>content, string word_query)
{
    int count = 0;
    for (const string& sc : content) {
        if (word_query == sc) {
            ++count;
        }
    }
    return count;
}

double TfIdf(vector<string>content, string word_query, double idf)
{
    double tf_idf = 0;
    int count = 0;
    for (const string& sc : content) {
        if (word_query == sc) {
            ++count;
        }
    }
    double tf = count * 1.0 / static_cast<int>(content.size());
    tf_idf += tf * idf;
    return tf_idf;
}

void TestRelevanceSearchDocuments()
{
    const int doc_id1 = 9;
    const string content1 = "fluffy black cat"s;
    const vector<int> ratings1 = { 1, 2, 3 };

    const int doc_id2 = 10;
    const string content2 = "orange parrot"s;
    const vector<int> ratings2 = { 1, 1 };

    const int doc_id3 = 11;
    const string content3 = "A cat with green eyes"s;
    const vector<int> ratings3 = { 2, 5, 3, 6 };

    const int doc_id4 = 12;
    const string content4 = "white big cat"s;
    const vector<int> ratings4 = { 1, 5, 1 };

    const string query = "white big parrot"s;

    SearchServer server;

    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
    server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, ratings4);

    vector<Document> matched_documents = server.FindTopDocuments(query);

    vector<string> split_query = SplitIntoWords(query);
    vector<string> split_content2 = SplitIntoWords(content2);
    vector<string> split_content4 = SplitIntoWords(content4);

    double tf_idf2 = 0;
    double tf_idf4 = 0;

    for (const string& sc : split_query)
    {
        int count2 = CountMatchWordInAllDocuments(split_content2, sc);
        int count4 = CountMatchWordInAllDocuments(split_content4, sc);
        double idf = log(4 * 1.0 / (count2 + count4));
        tf_idf2 += TfIdf(split_content2, sc, idf);
        tf_idf4 += TfIdf(split_content4, sc, idf);
    }

    ASSERT_EQUAL(matched_documents.size(), 2);
    ASSERT(abs(matched_documents[0].relevance - tf_idf4) < ACCURACY);
    ASSERT(abs(matched_documents[1].relevance - tf_idf2) < ACCURACY);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocuments);
    RUN_TEST(TestExcludeMinusWordsFromAddedDocumentContent);
    RUN_TEST(TestMatchDocuments);
    RUN_TEST(TestSortDocumentsByRelevance);
    RUN_TEST(TestCalculatingRatingDocuments);
    RUN_TEST(TestFilteringSearchResults);
    RUN_TEST(TestSearchDocementsByStatus);
    RUN_TEST(TestRelevanceSearchDocuments);
}

// --------- Окончание модульных тестов поисковой системы -----------


int main() {
    TestSearchServer();

    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id,
        DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    system("pause");
    return 0;
}