# cpp-search-server

Software implementation of a search server

## Core Features

0. Document search by keywords
1. Search result ranking based on TF-IDF statistical measure
2. Support for stop words and minus words. Stop words - ignored by the search system and do not affect search results. Minus words - documents containing such words will not be included in search results
3. Query queue creation and processing
4. Multi-threaded operation support

## Class Description

0. SearchServer — created with stop words specified (string or any container)
1. AddDocument — adds a document by ID, status, rating, and text
2. FindTopDocuments — returns documents sorted by TF-IDF relevance based on keywords, supports filtering, works in single-threaded and multi-threaded modes
3. RequestQueue — query queue, stores query history and results

## Usage

- Build using CMake
- Usage example is shown in main.cpp

## System Requirements

- C++17 and above (STL)

## Tech Stack

- C++17+  
- CMake 3.23.2  
- TBB (for multi-threading)  


