// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-
///////////////////////////////////////////////////////////////////////////////
//
// This file is a part of UPPAAL.
// Copyright (c) 2023, Aalborg University.
// All right reserved.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_FORMATS_PRINT_HPP
#define INCLUDE_FORMATS_PRINT_HPP

/** Idiomatic printing utilities.
 * Avoid including it into headers as it includes <iostream>, which slows down the builds.
 * Instead, declare printing functions in headers with <iosfwd>, and then implement them in cpp with <iostream>.
 * */

#include <iostream>
#include <string_view>

/** Print delimiter-separated elements.
 * @tparam View  the type of element view, can be array, std container, etc.
 * @param os the output stream
 * @param view the view over the elements
 * @param delim the delimeter to be put in between the elements into the output stream.
 * @return the same output stream reference after printing
 * */
template <typename View>
std::ostream& print_infix(std::ostream& os, const View& view, std::string_view delim = ",")
{
    auto b = std::begin(view), e = std::end(view);
    if (b != e) {
        os << *b;
        while (++b != e)
            os << delim << *b;
    }
    return os;
}

/** Print delimiter-separated elements through custom element printer.
 * @tparam View  the type of element view, can be array, std container, etc.
 * @tparam Print the print function type taking the output stream and the element in the view.
 * @param os the output stream
 * @param view the view over the elements
 * @param print the custom print function
 * @param delim the delimeter to be put in between the elements into the output stream.
 * @return the same output stream reference after printing
 * */
template <typename View, typename Print>
std::ostream& print_infix_p(std::ostream& os, const View& view, Print&& print, std::string_view delim = ",")
{
    auto b = std::begin(view), e = std::end(view);
    using E = typename std::iterator_traits<decltype(b)>::value_type;
    static_assert(std::is_invocable_v<Print, std::ostream&, const E&>, "print should accept istream and range element");
    if (b != e) {
        print(os, *b);
        while (++b != e)
            print(os << delim, *b);
    }
    return os;
}

/** Helper for infix printing into stream with infix separators.
 * For example, print "1,2,3": std::cout << infix{{1,2,3}};
 * @tparam View the view over some printable elements, can be array, container, etc.
 * */
template <typename View>
struct infix
{
    const View& view;
    std::string_view sep;
    infix(const View& view, std::string_view sep = ","): view{view}, sep{sep} {}
    friend std::ostream& operator<<(std::ostream& os, const infix& f) { return print_infix(os, f.view, f.sep); }
};

template <typename T>
infix(T&&) -> infix<T>;

template <typename View>
infix(View&&, std::string_view) -> infix<View>;

template <typename T>
infix(const std::initializer_list<T>&) -> infix<std::initializer_list<T>>;

template <typename T>
infix(const std::initializer_list<T>&, std::string_view) -> infix<std::initializer_list<T>>;

#endif  // INCLUDE_FORMATS_PRINT_HPP
