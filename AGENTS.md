# AGENTS

Notes for agents and contributors working in this repo.

## Repo map

- `split.h` — the entire library. One class template, `Split<S, T>`, plus a
  private input `Iterator`. Header-only.
- `test/test.cc` — smoke test exercising the static helper, `skip_blank`, and
  the iterator form.
- `CMakeLists.txt` — defines the `split` `INTERFACE` target (alias `split::split`) and a `ctest` test.
- `LICENSE` — MIT, Copyright (c) 2015-2019 Dubalu LLC.
- `README.md` — what it is, install, quick start.
- `docs/API.md` — the API reference and caveats; `docs/split.d2` is the README
  diagram's source.
- `ARCHITECTURE.md` — internal design (dispatch, iterator, skip_blank).

## Build & run the test

```sh
c++ -std=c++17 -I. test/test.cc -o test/test && ./test/test
# or:
cmake -B build && cmake --build build && ctest --test-dir build
```

The test prints `split OK: ...` and exits 0 on success; it uses `assert`, so
build without `NDEBUG`.

## Conventions

- C++17. The code relies on `std::string_view` and `[[fallthrough]]`.
- Header-only. Keep everything in `split.h`; there is no `.cc` to compile for the
  library itself.
- Indentation is tabs, matching the existing source.
- Use double quotes in code blocks per the docs' style.

## Invariants

- The iterator yields `std::string_view` values that alias the `Split`'s
  internal `str`, not the caller's original argument. The `Split` copies the
  source into `str` at construction.
- Those views are valid only while the `Split` object lives. Bind a `Split`
  built from a temporary to a named variable before iterating.
- The exhausted iterator state is both bounds equal to `S::npos`; that is what
  `end()` returns and what `operator bool()` checks.
- `search_func` is fixed at construction and never changes during iteration.

## How to extend

- Adding a search mode means adding a private member matching the
  `dispatch_search` signature (`size_type (Split::*)(size_type) const`), a new
  `Type` enum value, and a constructor case that points `search_func` at it and
  sets `inc` correctly (the advance past a match).
- Keep the eager static helpers in sync with any new mode if it should be
  reachable without constructing a `Split`.
- Add a matching assertion block to `test/test.cc` for any new behavior.

## Traps

- The `SKIP_BLANK_FIND` and `SKIP_BLANK_FIND_FIRST_OF` enum values set
  `skip_blank = false` (they keep blanks). The names read backwards.
- `inc` must match the mode: `sep.size()` for full-delimiter `FIND`, `1` for
  `FIND_FIRST_OF`. A wrong `inc` desynchronizes field boundaries.
- The static helpers do not emit a trailing empty field after a final
  delimiter; the iterator handles the trailing position separately. Don't assume
  the two paths produce identical field lists at the tail.
- `size()` walks the whole string (`std::distance(begin(), end())`); it is not a
  cheap accessor.

## Provenance

This code was extracted from
[Xapiand](https://github.com/Kronuz/Xapiand). Keep the MIT header intact when
editing `split.h`.
