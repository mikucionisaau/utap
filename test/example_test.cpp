#include <utap/utap.hpp>

#include <iostream>

int main(int argc, const char* args[])
{
    if (argc <= 2) {
        std::cerr << "Expecting two file paths as arguments: an input and output XML documents\n";
        return 1;
    }
    const auto* input_path = args[1];
    const auto* output_path = args[2];
    auto doc = UTAP::Document{};
    int res = parse_XML_file(input_path, doc, true, {});
    std::cout << "parse_XML_file(" << input_path << "): " << res << std::endl;
    if (res != 0)
        return 1;
    res = write_XML_file(output_path, doc);
    std::cout << "write_XML_file(" << output_path << "): " << res << std::endl;
    if (res != 0)
        return 1;
}
