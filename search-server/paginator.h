#pragma once


template<typename Iterator>
class IteratorRange
{
public:
    IteratorRange(Iterator range_begin, Iterator range_end) : begin_(range_begin), end_(range_end) {
    }

    auto begin() const {
        return begin_;
    }

    auto end() const {
        return end_;
    }

    auto size() const {
        return end_ - begin_;
    }
private:
    Iterator begin_;
    Iterator end_;

};

template<typename Iterator>
class  Paginator
{
public:

    Paginator(Iterator range_begin, Iterator range_end, size_t page_size) {

        auto it = range_begin;
        auto count_pages = std::distance(range_begin, range_end) / page_size;
        int check_count_page = std::distance(range_begin, range_end) % page_size;

        if (check_count_page == 0)
        {
            for (auto i = 0; i < count_pages; ++i)
            {
                pages_.push_back(IteratorRange(it, it + page_size));
                it += page_size;
            }
        }
        else
        {
            for (auto i = 0; i < count_pages; ++i)
            {
                pages_.push_back(IteratorRange(it, it + page_size));
                it += page_size;
            }
            pages_.push_back(IteratorRange(it, range_end));
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    auto size() const {
        return pages_.end() - pages_.begin();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;

};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
