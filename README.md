# split

A tiny, header-only **string splitter** for C++17. It walks a string once and
yields each field lazily as a `std::string_view` (no allocation), and also
offers static helpers that write fields straight into an output iterator.

Extracted from [Xapiand](https://github.com/Kronuz/Xapiand).

## Usage

```cpp
#include "split.h"

// Static helper: split into any container via an output iterator.
std::vector<std::string> out;
Split<>::split(std::string("alpha,beta,gamma"), ',', std::back_inserter(out));
// out == { "alpha", "beta", "gamma" }   (empty fields skipped by default)

// Iterator form: lazy, range-for friendly.
for (const auto& field : Split<>(std::string("x/y/z"), '/')) {
    // field is a std::string_view into the original string
}
```

Two split modes: split on a full delimiter string, or on *any character in a set*
(`find_first_of`), selectable via the constructor's `Type`. `skip_blank`
(default on) drops empty fields.

## Build & test

Header-only. To run the smoke test:

```sh
c++ -std=c++17 -I. test/test.cc -o test/test && ./test/test
# or: cmake -B build && cmake --build build && ctest --test-dir build
```

Requires C++17.

## Notes

- The lazy iterator yields views into the **original** string — keep it alive
  while iterating.

## License

MIT — see [LICENSE](LICENSE).
