#include <algorithm>
#include <iostream>
#include <optional>
#include <vector>

namespace impl {
enum class TokenType {
    tokConst,
    tokChar,
    tokPtr,
    tokArr,
    tokEnd,
    tokErr
};

class ILexer {
public:
    virtual TokenType next() = 0;
};

class Lexer : public ILexer {
    void skipTabs() {
        pos_ = std::find_if_not(pos_, end_, ::isspace);
    }

    bool tryTakeSeq(std::string_view seq) {
        auto [posStr, posWord] = std::mismatch(pos_, end_, seq.begin(), seq.end());
        if (posWord != seq.end())
            return false;

        pos_ = posStr;
        return true;
    }

    bool tryTakeWord(std::string_view word) {
        auto [posStr, posWord] = std::mismatch(pos_, end_, word.begin(), word.end());
        if (posWord != word.end() || std::isalpha(*posStr))
            return false;

        pos_ = posStr;
        return true;
    }

public:
    Lexer(std::string_view str):
        str_(str), pos_(str_.cbegin()), end_(str_.end()) {}

    TokenType next() override {
        using enum TokenType;
        skipTabs();
        if (pos_ == end_)
            return tokEnd;
        if (tryTakeWord("const"))
            return tokConst;
        if (tryTakeWord("char"))
            return tokChar;
        if (tryTakeSeq("*"))
            return tokPtr;
        if (tryTakeSeq("[]"))
            return tokArr;
        return tokErr;
    }

private:
    const std::string str_;
    std::string::const_iterator pos_;
    const std::string::const_iterator end_;
};

enum class ConstQual {
    Const,
    NonConst
};

// so primitive but useful for our purposes
struct Type {
    std::vector<ConstQual> quals_;
    bool isArray_ = false;

    void push(bool isConst) {
        quals_.push_back(isConst ? ConstQual::Const : ConstQual::NonConst);
    }

    void dump() const {
        std::cout << "char";
        auto condPrintConst = [](ConstQual qual) {
            if (qual == ConstQual::Const)
                std::cout << " const ";
        };

        ssize_t size = quals_.size();
        for (ssize_t i = 0; i < size - isArray_ - 1; i++) {
            condPrintConst(quals_[i]);
            std::cout << "*";
        }

        if (isArray_) {
            condPrintConst(quals_[size - 2]);
            std::cout << "[]";
        }
        condPrintConst(quals_[size - 1]);
        std::cout << std::endl;
    }
};

class Parser {
public:
    Parser(ILexer& lexer):
        lexer_(lexer) {}

    std::optional<Type> parse() {
        using enum TokenType;
        Type type;
        bool isConst = false;
        TokenType token = lexer_.next();

        if (token == tokConst) {
            isConst = true;
            token = lexer_.next();
        }

        if (token != tokChar)
            return std::nullopt;

        token = lexer_.next();
        while (token != tokErr && token != tokEnd) {
            if (token == tokConst) {
                isConst = true;
                token = lexer_.next();
            }

            if (token != tokPtr)
                break;

            type.push(isConst);
            isConst = false;
            token = lexer_.next();
        }

        if (token == tokArr) {
            type.push(isConst);
            isConst = false;
            type.isArray_ = true;
            token = lexer_.next();
        }
        
        if (token != tokEnd)
            return std::nullopt;

        type.push(isConst);
        return type;
    }

private:
    ILexer& lexer_;
};

auto getType(std::string_view typeStr) {
    Lexer lexer(typeStr);
    Parser parser(lexer);
    return parser.parse();
}

bool isConvertible(const Type &from, const Type &to) {
    if (from.quals_.size() != to.quals_.size())
        return false;

    if (to.isArray_ && !from.isArray_)
        return false;

    auto toEnd = to.quals_.end();
    auto [fromIt, toIt] = std::mismatch(from.quals_.begin(), from.quals_.end(), to.quals_.begin(), toEnd);
    if (toIt == toEnd)
        return true;
    return std::all_of(toIt, --toEnd, [](ConstQual qual) {
        return qual == ConstQual::Const;
    });
}
} // namespace impl

bool isConvertible(std::string_view fromStr, std::string_view toStr) {
    auto from = impl::getType(fromStr);
    if (!from) {
        std::cerr << "From type is incorrect!" << std::endl;
        return false;
    }

    auto to = impl::getType(toStr);
    if (!to) {
        std::cerr << "To type is incorrect!" << std::endl;
        return false;
    }

    return impl::isConvertible(*from, *to);
}

int main() {
    if (isConvertible("char const***[]", "const char **const *const* const"))
        std::cout << "Convertible!" << std::endl;
    else
        std::cout << "Not convertible!" << std::endl;

    return 0;
}
