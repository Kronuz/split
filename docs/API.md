# split API reference

```cpp
template <typename S = std::string, typename T = char>
class Split;
```

`S` is the string type that backs the splitter; `T` is the separator type
(a single `char` or a string of type `S`).

## Constructors

```cpp
Split() = default;
Split(String&& str, String&& sep, Type type = Type::FIND);   // string delimiter
Split(String&& str, Sep&&    sep, Type type = Type::FIND);   // char delimiter
```

The two non-default constructors are SFINAE-constrained: the string-delimiter
overload requires both arguments to decay to the same type as `S`; the
char-delimiter overload requires the separator to decay to `char`. Both take the
source string by forwarding reference and store a copy in `str`.

## Type modes

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

## Iterators

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

## Static helpers

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
