#pragma once
#include <algorithm>
#include <string>
#include <utility>

namespace cow {
// todo: traits, char allocator, control block allocator
template <class CharT>
class BasicString {
public:
    using value_type = CharT;
    using pointer = CharT*;
    using const_pointer = const CharT*;
    using reference = CharT&;
    using const_reference = const CharT&;
    using const_iterator = const_pointer;
    using iterator = pointer;
    using const_reverse_iterator = std::reverse_iterator<const_pointer>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using string_view = std::basic_string_view<CharT>;
    static constexpr size_type npos = size_type(-1);

private:
    class ControlBlock {
        ControlBlock(size_type size) :
            size_(size),
            data_(new CharT[capacity_])
        {}

        ControlBlock(string_view str):
            size_(str.size()),
            capacity_(std::max(defaultCapacity, size_)),
            data_(new CharT[capacity_]) {
            std::copy(str.begin(), str.end(), data_);
        }

        ControlBlock(const ControlBlock& block): 
            ControlBlock(string_view(block.data_, block.capacity_)) {}

        ~ControlBlock() {
            delete[] data_;
        }

    public:
        ControlBlock& operator=(const ControlBlock&) = delete;
        ControlBlock(ControlBlock&&) = delete;
        ControlBlock& operator=(ControlBlock&&) = delete;

        static ControlBlock* create(size_type size) {
            return new ControlBlock(size);
        }

        static ControlBlock* create(string_view str) {
            return new ControlBlock(str);
        }

        ControlBlock* link() const {
            links_++;
            return const_cast<ControlBlock*>(this);
        }

        void unlink() const {
            if (--links_ == 0)
                delete this;
        }

        ControlBlock* clone() const {
            return new ControlBlock(*this);
        }

        size_type getNumLinks() const {
            return links_;
        }

        reference at(size_type i) {
            return data_[i];
        }

        const_reference at(size_type i) const {
            return data_[i];
        }

        iterator begin() {
            return data_;
        }

        iterator end() {
            return data_ + size_;
        }

        const_iterator cbegin() const {
            return data_;
        }

        const_iterator cend() const {
            return data_ + size_;
        }

        reverse_iterator rbegin() {
            return std::make_reverse_iterator(data_ + size_);
        }

        reverse_iterator rend() {
            return std::make_reverse_iterator(data_);
        }

        const_reverse_iterator crbegin() const {
            return std::make_reverse_iterator(data_ + size_);
        }

        const_reverse_iterator crend() const {
            return std::make_reverse_iterator(data_);
        }
        
        size_type find_first_of(string_view str, size_type pos) const {
            auto strBegin = str.begin(), strEnd = str.end();
            for (size_type i = pos; i < size_; i++)
                if (std::any_of(strBegin, strEnd, at(i)))
                    return i;
            return npos;
        }

        size_type find_first_not_of(string_view str, size_type pos) const {
            auto strBegin = str.begin(), strEnd = str.end();
            for (size_type i = pos; i < size_; i++)
                if (std::none_of(strBegin, strEnd, at(i)))
                    return i;
            return npos;
        }

        size_type findSubstr(string_view str, size_type pos) const {
            auto strSize = str.size();
            auto strBegin = str.begin(), strEnd = str.end();
            for (auto it : *this) {
                auto [strIt, myIt] = std::mismatch(strBegin, strEnd, it);
                if (strIt == strEnd)
                    return std::distance(begin(), it);
            }
            return npos;
        }

    private:
        static constexpr size_type defaultCapacity = 16;
        size_type size_, capacity_ = defaultCapacity;
        mutable size_type links_ = 1;
        CharT* data_;
    };

private:
    void cloneBlockIfNeeded() {
        if (storage_->getNumLinks() > 1)
            storage_ = storage_->clone();
    }

public:
    BasicString(size_t size = 0):
        storage_(ControlBlock::create(size)) {}

    BasicString(string_view str):
        storage_(ControlBlock::create(str)) {}

    BasicString(const char *str):
        BasicString(string_view(str)) {}

    BasicString(const BasicString &str):
        storage_(str.storage_->link()) {}

    BasicString& operator=(const BasicString &str) {
        storage_->unlink();
        storage_ = str.storage_->link();
        return *this;
    }

    BasicString(BasicString &&str):
        storage_(std::exchange(str.storage_, nullptr)) {}

    BasicString& operator=(BasicString &&str) {
        std::swap(storage_, str.storage_);
        return *this;
    }

    ~BasicString() {
        if (storage_)
            storage_->unlink();
    }

    reference operator[](size_type i) {
        cloneBlockIfNeeded();
        return storage_->at(i);
    }

    const_reference operator[](size_type i) const {
        return storage_->at(i);
    }

    iterator begin() {
        return storage_->begin();
    }

    iterator end() {
        return storage_->end();
    }

    const_iterator cbegin() const {
        return storage_->cbegin();
    }

    const_iterator cend() const {
        return storage_->cend();
    }

    reverse_iterator rbegin() {
        return storage_->rbegin();
    }

    reverse_iterator rend() {
        return storage_->rend();
    }

    const_reverse_iterator crbegin() const {
        return storage_->crbegin();
    }

    const_reverse_iterator crend() const {
        return storage_->crend();
    }

    size_type find_first_of(string_view str, size_type pos = 0) const {
        return storage_->find_first_of(str, pos);
    }

    size_type find_first_not_of(string_view str, size_type pos) const {
        return storage_->find_first_not_of(str, pos);
    }

    size_type findSubstr(string_view str, size_type pos = 0) const {
        return storage_->findSubstr(str, pos);
    }

private:
    ControlBlock* storage_;
};

template <typename StringT>
class BasicTokenizer {
    // sorry for no iterator traits...
    struct Iterator {
        using StrIt = StringT::const_iterator;
        using View = StringT::string_view;

        Iterator(StrIt begin, StrIt end, StrIt sepBegin, StrIt sepEnd):
            beginTok_(begin), endTok_(begin), end_(end), sepBegin_(sepBegin), sepEnd_(sepEnd) {
            operator++();
        }

        View operator*() const {
            return View(beginTok_, endTok_);
        }

        void operator++() {
            using Ref = typename StringT::const_reference;
            beginTok_ = std::find_if(endTok_, end_, [begin = sepBegin_, end = sepEnd_](Ref ch1) {
                return std::none_of(begin, end, [ch1](Ref ch2){
                    return ch1 == ch2;
                });
            });
            endTok_ = std::find_first_of(beginTok_, end_, sepBegin_, sepEnd_);
        }

        bool operator==(const Iterator& other) const {
            return beginTok_ == other.beginTok_;
        }

    private:
        StrIt beginTok_, endTok_;
        const StrIt end_, sepBegin_, sepEnd_;
    };

public:
    BasicTokenizer(StringT str, StringT sep):
        str_(std::move(str)), sep_(std::move(sep)) {}

    using iterator = Iterator;

    iterator begin() const {
        return Iterator(str_.cbegin(), str_.cend(), sep_.cbegin(), sep_.cend());
    }

    iterator end() const {
        return Iterator(str_.cend(), str_.cend(), sep_.cbegin(), sep_.cend());
    }

private:
    const StringT str_, sep_;
};

using String = BasicString<char>;
using Tokenizer = BasicTokenizer<String>;

} // namespace cow
