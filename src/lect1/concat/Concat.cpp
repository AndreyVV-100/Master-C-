#include <iostream>
#include <tuple>
#include <string>

namespace impl {

template <size_t Start, typename ... Args, size_t ... Nums>
auto getTuplePart(std::tuple<Args ...> &strings, std::index_sequence<Nums ...>) {
    return std::make_tuple(std::get<Start + Nums>(strings) ...);
}

template <typename ... Args>
std::string stringTwine(std::tuple<Args ...> strings);

template <>
std::string stringTwine(std::tuple<std::string_view> string) {
    return std::string(std::get<0>(string));
}

template <>
std::string stringTwine(std::tuple<std::string_view, std::string_view> strings) {
    std::string result(std::get<0>(strings));
    result += std::get<1>(strings);
    return result;
}

template <typename ... Args>
std::string stringTwine(std::tuple<Args ...> strings) {
    constexpr size_t N = sizeof ... (Args);
    return stringTwine(std::make_tuple<std::string_view, std::string_view>(
                        stringTwine(getTuplePart<0>(strings, std::make_index_sequence<N / 2>())),
                        stringTwine(getTuplePart<N / 2>(strings, std::make_index_sequence<N / 2 + N % 2>()))));
}

} // namespace impl

template <typename ... Args>
std::string stringTwine(Args&& ...args) {
    return impl::stringTwine(std::make_tuple(std::string_view(std::forward<Args>(args)) ...));
}

int main() {
    std::cout << stringTwine("Hello ", "from ", "so ", "beautiful ", "and ", "sunny ", "Dolgoprudniy", "!!!!!", "\n");
}
