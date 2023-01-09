#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cassert>
#include <stdexcept>
#include <numeric>

#include "search_server.h"

using namespace std;

template<typename T>
void Print(ostream& out, const T& container) {
    bool first = true;
    for (const auto& entry : container) {
        if (!first) {
            out << ", "s;
        }
        first = false;
        out << entry;
    }
}

template<typename Element>
ostream& operator<<(ostream& out, vector<Element> vector) {
    Print(out, vector);
    return out;
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
    const string& hint);

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

void TestExcludeStopWordsFromAddedDocumentContent();
void TestAddDocuments();
void TestExcludeMinusWordsFromAddedDocumentContent();
void TestMatchDocuments();
void TestSortDocumentsByRelevance();
void TestCalculatingRatingDocuments();
void TestFilteringSearchResults();
void TestSearchDocementsByStatus();
int CountMatchWordInAllDocuments(vector<string>content, string word_query);
double TfIdf(vector<string>content, string word_query, double idf);
void TestRelevanceSearchDocuments();
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();
