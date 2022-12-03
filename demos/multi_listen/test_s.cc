#include <string>
#include <iostream>

void print_with_zeros(const std::string& note, std::string const& s)
{
    std::cout << note;
    for (const char c : s) {
        (c ? std::cout << c : std::cout << "₀");
    }
    std::cout << " (size = " << s.size() << ")\n";
}

int main()
{
    using namespace std::string_literals;

    std::string s1 = "abc\0\0def";
    std::string s2 = "abc\0\0def"s + "op:"s ;
    print_with_zeros("s1: ", s1);
    print_with_zeros("s2: ", s2);

    std::cout << "abcdef"s.substr(1,4) << '\n';
}
