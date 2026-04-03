# cpp-search-server

Software implementation of a search server

## Core Features

1. Document search by keywords
2. Search result ranking based on TF-IDF statistical measure
3. Support for stop words and minus words. Stop words - ignored by the search system and do not affect search results. Minus words - documents containing such words will not be included in search results
4. Query queue creation and processing
5. Multi-threaded operation support

## Class Description

1. SearchServer — created with stop words specified (string or any container)
2. AddDocument — adds a document by ID, status, rating, and text
3. FindTopDocuments — returns documents sorted by TF-IDF relevance based on keywords, supports filtering, works in single-threaded and multi-threaded modes
4. RequestQueue — query queue, stores query history and results

## Usage

- Build using CMake
- Usage example is shown in main.cpp

## System Requirements

- C++17 and above (STL)

## Tech Stack

- C++17+  
- CMake 3.23.2  
- TBB (for multi-threading)  


