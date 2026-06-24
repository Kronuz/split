# split

A tiny, header-only string splitter for C++17.

## What it is

`split` is a single-header utility that breaks a string into fields. It walks
the string once and yields each field lazily as a `std::string_view` into the
original buffer, so the iterator form allocates nothing. It also ships static
helpers that write fields straight into an output iterator when you'd rather
collect them into a container.

## When to use it / when not

Use it when you want a small, dependency-free splitter that either streams
fields without copying (the iterator) or fills a container in one call (the
static helpers). It handles both "split on a full delimiter string" and "split
on any character in a set."

Skip it if you need regex splitting, locale-aware tokenization, or a streaming
parser over data that doesn't fit in a single contiguous string. For the
zero-copy iterator you also have to keep the source string alive for as long as
you read the views (see Notes & caveats).

## Install

Header-only. Copy `split.h` somewhere on your include path and include it:

```cpp
#include "split.h"
```

With CMake there's an `INTERFACE` target (`split`) you can link against. To pull
it in via `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
  split
  GIT_REPOSITORY https://github.com/Kronuz/split.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(split)

target_link_libraries(your_target PRIVATE split)
```

The `split` target adds the include directory and requests `cxx_std_17`.

## Usage

All examples assume `#include "split.h"`.

### Static helper into an output iterator

```cpp
#include <iterator>
#include <string>
#include <vector>

std::vector<std::string> out;
Split<>::split(std::string("alpha,beta,gamma"), ',', std::back_inserter(out));
// out == { "alpha", "beta", "gamma" }
```

### Lazy iterator, range-for

```cpp
// Each field is a std::string_view into the original string.
Split<> sp(std::string("x/y/z"), '/');
for (const auto& field : sp) {
    // use field ...
}
```

### Full-delimiter mode vs. find-first-of mode

```cpp
// Default Type::FIND splits on the whole delimiter string "::".
std::vector<std::string> a;
Split<>::split(std::string("a::b::c"), std::string("::"), std::back_inserter(a));
// a == { "a", "b", "c" }

// split_first_of splits on ANY character in the set, here ',' or ';'.
std::vector<std::string> b;
Split<>::split_first_of(std::string("a,b;c"), std::string(",;"), std::back_inserter(b));
// b == { "a", "b", "c" }
```

For a single-char delimiter both modes behave identically.

### skip_blank

```cpp
// skip_blank defaults to true, so empty fields are dropped.
std::vector<std::string> kept;
Split<>::split(std::string("a,,b,"), ',', std::back_inserter(kept));
// kept == { "a", "b" }

// Pass false to preserve empty fields.
std::vector<std::string> all;
Split<>::split(std::string("a,,b,"), ',', std::back_inserter(all), false);
// all == { "a", "", "b" }   (no trailing empty; see Notes & caveats)
```

## API reference

```cpp
template <typename S = std::string, typename T = char>
class Split;
```

`S` is the string type that backs the splitter; `T` is the separator type
(a single `char` or a string of type `S`).

### Constructors

```cpp
Split() = default;
Split(String&& str, String&& sep, Type type = Type::FIND);   // string delimiter
Split(String&& str, Sep&&    sep, Type type = Type::FIND);   // char delimiter
```

The two non-default constructors are SFINAE-constrained: the string-delimiter
overload requires both arguments to decay to the same type as `S`; the
char-delimiter overload requires the separator to decay to `char`. Both take the
source string by forwarding reference and store a copy in `str`.

### Type modes

```cpp
enum class Type : uint8_t {
    FIND,                      // split on the full delimiter string (str.find)
    FIND_FIRST_OF,             // split on any char in the set (str.find_first_of)
    SKIP_BLANK_FIND,           // FIND, but keep blank fields
    SKIP_BLANK_FIND_FIRST_OF,  // FIND_FIRST_OF, but keep blank fields
};
```

`Type` picks the search strategy via a member-function-pointer
(`dispatch_search`). `FIND` matches the whole delimiter string;
`FIND_FIRST_OF` matches any single character in the separator set. Note that the
two `SKIP_BLANK_*` variants do not skip blanks; they set the internal
`skip_blank` flag to `false`, so blank fields are kept. The default constructor
state has `skip_blank == true`. For a single-character delimiter `FIND` and
`FIND_FIRST_OF` produce the same fields.

### Iterators

```cpp
iterator       begin();
const_iterator begin() const;   // also cbegin()
iterator       end();
const_iterator end()   const;   // also cend()
```

`iterator` is an input iterator over `std::string_view`. Dereferencing returns a
view into the source string; incrementing advances to the next field. The
iterator also exposes `last()` (true on the final field) and an `explicit
operator bool()` (false once exhausted). `Split` additionally offers `empty()`,
`size()` (distance from begin to end, which walks the string), and `get_str()`.

### Static helpers

```cpp
template <typename OutputIt>
static void split(const S& str, const S& delimiter, OutputIt out, bool skip_blank = true);
template <typename OutputIt>
static void split(const S& str, char delimiter, OutputIt out, bool skip_blank = true);

template <typename OutputIt>
static void split_first_of(const S& str, const S& delimiter, OutputIt out, bool skip_blank = true);
template <typename OutputIt>
static void split_first_of(const S& str, char delimiter, OutputIt out, bool skip_blank = true);
```

`split` matches the whole `delimiter`; `split_first_of` matches any single
character in `delimiter`. Each writes `S` substrings (not views) into `out`,
which is any output iterator such as `std::back_inserter(vec)`. `skip_blank`
defaults to `true` and drops empty fields. The char overloads treat the
delimiter as a single character; `split_first_of(char)` just forwards to
`split(char)` since a one-character set is the same as a one-character
delimiter.

Note an asymmetry between the helpers and the iterator: a trailing empty field
(the segment after a final delimiter) is never emitted by the static helpers
because they only emit the tail when `prev < str.size()`. The iterator handles
the trailing position through its own `npos` logic.

## Build & test

Header-only, so there's nothing to compile for use. To run the smoke test:

```sh
c++ -std=c++17 -I. test/test.cc -o test/test && ./test/test
# or with CMake:
cmake -B build && cmake --build build && ctest --test-dir build
```

Requires a C++17 compiler.

## Notes & caveats

- The lazy iterator yields `std::string_view` values that alias the original
  string. Keep the source alive for as long as you read those views.
- The `Split` object stores a copy of the source string in `str`, so the views
  are valid for the lifetime of the `Split` object, not the original argument.
  When you build a `Split` from a temporary (as in the examples), bind it to a
  named variable before iterating so the copy outlives the loop.
- The `SKIP_BLANK_FIND` / `SKIP_BLANK_FIND_FIRST_OF` enum names are
  counterintuitive: they keep blank fields rather than skipping them.
- The static helpers do not emit a trailing empty field after a final
  delimiter (see API reference).

## Provenance

Extracted from [Xapiand](https://github.com/Kronuz/Xapiand).

## License

MIT. Copyright (c) 2015-2019 Dubalu LLC. See [LICENSE](LICENSE).
