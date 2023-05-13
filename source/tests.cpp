#include "tests.h"


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

// -------- Начало модульных тестов поисковой системы ----------

/*void TestExcludeStopWordsFromAddedDocumentContent() {
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
    const int doc_id1 = 43;
    const string content1 = "A black cat with green eyes"s;
    const vector<int> ratings1 = { 1, 2, 3, 4 };

    const int doc_id2 = 44;
    const string content2 = "white cat"s;
    const vector<int> ratings2 = { 2, 1 };

    SearchServer server;
    const string query = "big white cat"s;

    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    tuple<vector<string>, DocumentStatus> match_doc1 = server.MatchDocument(query, doc_id1);
    vector<string> expected_match_doc1 = { "cat"s };
    ASSERT_EQUAL(get<0>(match_doc1), expected_match_doc1);

    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    tuple<vector<string>, DocumentStatus> match_doc2 = server.MatchDocument(query, doc_id2);
    vector<string> expected_match_doc2 = { "cat"s, "white"s };
    ASSERT_EQUAL(get<0>(match_doc2), expected_match_doc2);

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
}*/