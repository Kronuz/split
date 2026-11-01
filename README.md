# split

Part of [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries), a collection of small, reusable C++ libraries.

[![CI](https://github.com/Kronuz/split/actions/workflows/ci.yml/badge.svg)](https://github.com/Kronuz/split/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Header-only](https://img.shields.io/badge/header--only-yes-green.svg)

Header-only string splitter for C++17: lazy std::string_view fields, no allocation while iterating.
`Split` holds the source and the separator and finds one field per step of its
iterator, so a range-for walks the fields without building a vector. Static
helpers fill any container through an output iterator in one call. Reach for it
to walk paths, key lists and other delimited text where allocations count.

## ✨ Highlights

- **Lazy fields.** Each step of the iterator is one `find` or `find_first_of`
  and yields a `std::string_view`.
- **Views or a copy.** `Split<std::string_view>` reads the caller's buffer;
  `Split<>` (`std::string`) keeps its own copy, so it can own a temporary.
- **One-call collection.** `Split<>::split` and `split_first_of` write fields into
  any output iterator.
- **Two modes.** Split on a whole delimiter (`"::"`) or on any character of a
  set (`",;"`).
- **Blank fields your way.** Empty fields are skipped by default, or kept.
- **One header, no dependencies.**

## 🚀 Quick start

```cpp
#include "split.h"

#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

int main() {
	std::string_view line = "usr/local//bin";
	Split<std::string_view> path(line, '/');            // views into line, no copy
	for (std::string_view part : path) {
		std::printf("[%.*s]", int(part.size()), part.data());
	}
	std::printf("\n");                                   // [usr][local][bin]

	std::vector<std::string> words;
	Split<>::split_first_of(std::string("a,b;;c"), std::string(",;"), std::back_inserter(words));
	std::printf("%zu\n", words.size());                  // 3: blank fields are skipped
}
```

[`examples/demo.cc`](examples/demo.cc) contrasts the two modes, blank fields, and
joining with the iterator's `last()`.

## 📦 Installation

Requires a C++17 compiler and CMake 3.14 or newer; no dependencies. CI builds it
on Linux (x86_64, arm64), macOS (arm64, x86_64), and FreeBSD, plus ASan, UBSan,
and TSan builds.

```cmake
include(FetchContent)
FetchContent_Declare(split
  GIT_REPOSITORY https://github.com/Kronuz/split.git
  GIT_TAG        22add3f5435b1fd354fa3589d3d92f052400ca8f)  # or main for the tip
FetchContent_MakeAvailable(split)

target_link_libraries(my_app PRIVATE split::split)
```

```cpp
#include "split.h"
```

Without CMake, copy `split.h` onto your include path.

## 🎯 When to use it

Use it to walk delimited text (paths, key lists, simple field lists) once,
without materializing the fields.

Not for:

- **CSV.** There is no quoting and no escaping.
- **Patterns.** It splits on a fixed delimiter or a character set, not a regular
  expression, and knows nothing of locales.
- **Fields that outlive the splitter.** The views point into the `Split` (or, for
  `Split<std::string_view>`, into your buffer). Bind a `Split` built from a
  temporary to a variable before iterating it.
- **Counting cheaply.** `size()` walks the whole string.
- **Short strings, when abseil is at hand.** `absl::StrSplit` is 1.2x (M4 Pro)
  to 1.7x (i9) faster on 64 bytes.

## ⚡ Performance

`bench/compare.cc` times `Split` against hand-written loops (`memchr` for a
character, `string_view::find` for `"::"`, `find_first_of` for `",;"`) and
`absl::StrSplit`, on comma-separated text of 1 to 15 character fields, one in
ten of them empty. Both sides return the same fields, checked before each row is
timed. Time per call, the best of seven samples of at least 8 ms; `-O3`, Apple
clang 17, abseil 20260817.0:

| workload | Apple M4 Pro: `Split` / hand loop / absl | Intel Core i9-9980HK: `Split` / hand loop / absl |
|---|---:|---:|
| `split()` on `,` into a vector, 64 B | 75 / 80 / 62 ns | 228 / 223 / 129 ns |
| `split()` on `,`, 1 KB | 812 / 819 / 989 ns | 1.22 / 1.20 / 1.53 us |
| `split()` on `,`, 64 KB | 54.8 / 56.0 / 75.1 us | 68.1 / 62.1 / 89.0 us |
| `split()` on `"::"`, 1 KB | 910 / 897 / 1,256 ns | 1.66 / 1.60 / 2.11 us |
| `split_first_of()` on `,;`, 1 KB | 908 / 1,046 / 972 ns | 1.80 / 1.91 / 2.43 us |
| `Split<std::string_view>` iterator, 1 KB | 726 / 745 / 909 ns | 1.12 / 0.80 / 1.08 us |
| `Split<std::string_view>` iterator, 64 KB | 52.3 / 55.1 / 75.3 us | 81.3 / 55.4 / 73.9 us |
| `Split<std::string>` iterator, 1 KB | 1,007 / 745 / 912 ns | 1.49 / 0.80 / 1.08 us |

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release -DSPLIT_BUILD_BENCH=ON
cmake --build build --target split_compare
./build/split_compare
```

The one-call helpers are level with the hand loop and, from 1 KB up, take 0.7x
to 0.9x of `absl::StrSplit`'s time. The iterator keeps a look-ahead position, so
it costs 1.0x of a hand loop on the M4 Pro and 1.4x to 1.5x on the i9.
`Split<std::string>` copies its source: 1.3x to 1.4x the time of the view form
at 1 KB, 1.9x at 64 bytes.

## 📚 Documentation

- [docs/API.md](docs/API.md): constructors, the four modes, iterators, the static
  helpers, and caveats (including the `SKIP_BLANK_*` names, which keep blanks).
- [ARCHITECTURE.md](ARCHITECTURE.md): the search dispatch, the lazy iterator and
  `skip_blank`.
- [AGENTS.md](AGENTS.md): repository map, invariants, and traps.
- [Kronuz C++ Libraries](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md#split): the catalog entry.

## 🛠️ Building and testing

```sh
cmake -B build
cmake --build build
ctest --test-dir build
```

`./build/split_demo` runs the demo. Add `-DSPLIT_BUILD_BENCH=ON` to build
`split_compare`; with abseil installed it adds the `absl::StrSplit` column, and
`ctest` runs its `--check` mode. The iterator derives from the deprecated
`std::iterator`, so builds print `-Wdeprecated-declarations` warnings.

## 🤝 Contributing

Issues and pull requests are welcome. Read [AGENTS.md](AGENTS.md) and the
catalog's [rules of engagement](https://github.com/Kronuz/cpp-libraries/blob/main/AGENTS.md#rules-of-engagement)
first; CI must pass on every platform.

## 📄 License

MIT, Copyright (c) 2026 Germán Méndez Bravo and Copyright (c) 2015-2020 Dubalu
LLC. See [LICENSE](LICENSE). Extracted from [Xapiand](https://github.com/Kronuz/Xapiand).
