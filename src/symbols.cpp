// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2002-2006 Uppsala University and Aalborg University.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA
*/

#include "utap/symbols.hpp"

#include "utap/expression.hpp"
#include "utap/range.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <vector>

#include <cstdlib>

namespace UTAP {
using namespace Constants;

//////////////////////////////////////////////////////////////////////////

struct Symbol::Data : std::enable_shared_from_this<Data>
{
    Frame::Data* frame_data = nullptr;  // Uncounted pointer to containing frame // TODO: consider removing
    Type type;                     // The type of the symbol
    void* user = nullptr;          // User data
    std::string name;              // The name of the symbol
    position_t position;           // the position of the symbol definition in the original document
    Data(Frame::Data* frame_data, Type type, void* user, std::string_view name, position_t position):
        frame_data{frame_data}, type{std::move(type)}, user{user}, name{name}, position{position}
    {}
};

Symbol::Symbol(Frame& frame, Type type, std::string_view name, position_t position, void* user)
{
    data = std::make_shared<Data>(frame.data.get(), std::move(type), user, name, position);
}

Symbol::~Symbol() noexcept = default; ///< hidden due to pImpl encapsulation

/* Get frame this symbol belongs to */
Frame Symbol::get_frame() const { return Frame(data->frame_data); }

/* Returns the type of this symbol. */
const Type& Symbol::get_type() const { return data->type; }

void Symbol::set_type(Type type) { data->type = std::move(type); }

position_t Symbol::get_position() const { return data->position; }

/* Returns the user data of this symbol */
void* Symbol::get_data() { return data->user; }

/* Returns the user data of this symbol */
const void* Symbol::get_data() const { return data->user; }

/* Returns the name (identifier) of this symbol */
const std::string& Symbol::get_name() const { return data->name; }

void Symbol::set_name(std::string name) { data->name = std::move(name); }

std::ostream& operator<<(std::ostream& o, const Symbol& t)
{
    return o << t.get_type() << " " << t.get_name();
}

//////////////////////////////////////////////////////////////////////////

struct Frame::Data : std::enable_shared_from_this<Data>
{
    using Ptr = std::shared_ptr<Data>;
    Ptr parent;                                           ///< The parent frame data
    std::vector<Symbol> symbols;                          ///< The symbols in the frame
    std::map<std::string, int32_t, std::less<>> mapping;  ///< Mapping from names to indices
    explicit Data(Ptr p): parent{std::move(p)} {}
    bool has_parent() const { return parent != nullptr; }
};

Frame::Frame(Data* data): data{data->shared_from_this()} { }

/* Destructor */
Frame::~Frame() noexcept = default;

/* Returns the number of symbols in this frame */
uint32_t Frame::get_size() const { return static_cast<uint32_t>(data->symbols.size()); }
bool Frame::empty() const { return data->symbols.empty(); }

/* Returns the Nth symbol in this frame (counting from 0) */
Symbol Frame::get_symbol(uint32_t n) const { return data->symbols[n]; }

/* Returns the Nth symbol in this frame (counting from 0) */
Symbol& Frame::operator[](uint32_t n) { return data->symbols[n]; }

/* Returns the Nth symbol in this frame (counting from 0) */
const Symbol& Frame::operator[](uint32_t n) const { return data->symbols[n]; }

Frame::const_iterator Frame::begin() const { return std::begin(data->symbols); }
Frame::const_iterator Frame::end() const { return std::end(data->symbols); }
Frame::iterator Frame::begin() { return std::begin(data->symbols); }
Frame::iterator Frame::end() { return std::end(data->symbols); }

/* Adds a symbol of the given name and type to the frame */
Symbol Frame::add_symbol(std::string_view name, Type type, position_t position, void* user)
{
    auto symbol = Symbol{*this, std::move(type), name, position, user};
    data->symbols.push_back(symbol);
    if (!name.empty()) {
        data->mapping[symbol.get_name()] = static_cast<int32_t>(data->symbols.size() - 1);
    }
    return symbol;
}

/** Add symbol. Notice that the symbol will be in two frames at the
    same time, but the symbol will only "point back" to the first
    frame it was added to.
*/
void Frame::add(Symbol symbol)
{
    data->symbols.push_back(std::move(symbol));
    const auto& symb = data->symbols.back();
    if (!symb.get_name().empty())
        data->mapping[symb.get_name()] = static_cast<int32_t>(data->symbols.size() - 1);
}

/** Add all symbols in the given frame. Notice that the symbols will
    be in two frames at the same time, but the symbol will only "point
    back" to the first frame it was added to.
*/
void Frame::add(const Frame& frame)
{
    for (const auto& symbol : frame)
        add(symbol);
}

/** Move all symbols from this to the given frame.
    Notice that the symbols will be updated to point to the given frame,
    while this frame will be emptied and ready for recycling.
*/
void Frame::move_to(Frame& frame)
{
    auto symbols = std::move(data->symbols);
    for (auto&& symbol : symbols) {
        symbol.data->frame_data = frame.data.get();
        frame.add(std::move(symbol));
    }
    data->symbols.clear();
    data->mapping.clear();
}

/** removes the given symbol*/
void Frame::remove(const Symbol& s)
{
    auto symbols = std::move(data->symbols);
    data->symbols.clear();
    data->mapping.clear();
    for (auto&& symbol : symbols) {
        if (symbol != s) {
            symbol.data->frame_data = data.get();
            add(std::move(symbol));
        }
    }
}

std::optional<uint32_t> Frame::get_index_of(std::string_view name) const
{
    auto res = std::optional<uint32_t>{};
    if (auto it = data->mapping.find(name); it != data->mapping.end())
        res = it->second;
    return res;
}

std::optional<uint32_t> Frame::get_index_of(const Symbol& symbol) const
{
    auto res = std::optional<uint32_t>{};
    auto it = std::find(std::begin(data->symbols), std::end(data->symbols), symbol);
    if (it != std::end(data->symbols))
        res = std::distance(std::begin(data->symbols), it);
    return res;
}

/**
   Resolves the name in this frame or the parent frame and
   returns the corresponding symbol.
*/
bool Frame::resolve(std::string_view name, Symbol& symbol) const
{
    auto idx = get_index_of(name);
    if (!idx)
        return (data->has_parent() ? get_parent().resolve(name, symbol) : false);
    symbol = data->symbols[*idx];
    return true;
}

/// Returns the parent frame
Frame Frame::get_parent() const
{
    if (!data->has_parent())
        throw NoParentException{};
    return Frame{data->parent.get()};
}

/// Returns true if this frame has a parent
bool Frame::has_parent() const { return data->has_parent(); }

/// Creates and returns a new frame without a parent
Frame Frame::make()
{
    auto d = std::make_shared<Data>(nullptr);
    return Frame{d.get()};
}

/// Creates and returns new frame with the given parent
Frame Frame::make_sub()
{
    auto d = std::make_shared<Data>(this->data);
    return Frame{d.get()};
}

std::ostream& operator<<(std::ostream& os, const Frame& t)
{
    os << "{";
    const auto e = std::end(t);
    if (auto b = std::begin(t); b != e) {
        os << *b;
        while (++b != e)
            os << ", " << *b;
    }
    return os << "}";
}

} // namespace UTAP