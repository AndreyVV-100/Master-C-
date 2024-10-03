#include "Cow.hpp"
#include <iostream>

int main() {
    cow::String str = ";;Hello|world||-foo--bar;yow;baz|";
    cow::String sep("-;|");

    cow::Tokenizer tokens(str, sep);
    for (auto tok : tokens)
        std::cout << "<" << tok << "> ";
    std::cout << std::endl;
}