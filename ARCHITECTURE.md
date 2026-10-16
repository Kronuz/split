# Architecture

`split` is one header, `split.h`, holding a single class template
`Split<S = std::string, T = char>`. This document covers how the lazy iterator
walks the string, how the search mode is selected, and the trade-offs the design
makes.

## Search-mode dispatch

A `Split` stores the source string (`str`), the separator (`sep`), a
`skip_blank` flag, an `inc` step, and a member-function pointer `search_func`
of type `dispatch_search`. Two private members implement the two
strategies:

- `find(pos)` calls `str.find(sep, pos)` to locate the full delimiter string.
- `find_first_of(pos)` calls `str.find_first_of(sep, pos)` to locate any single
  character in the separator set.

The constructor inspects the `Type` argument and points `search_func` at one of
these. A private `next(pos)` member just
dereferences the pointer: `(this->*search_func)(pos)`. Using a
member-function pointer instead of a branch on every search keeps the hot loop
free of a per-call `Type` check; the decision is made once at construction.

`inc` is the amount to advance past a match: `sep.size()` for full-delimiter
mode and `1` for find-first-of mode, since a
character-set match consumes exactly one character.

## The lazy iterator

`Iterator<V>` is an input iterator. It holds a back-pointer to
the owning `Split`, the `[start, end)` bounds of the current field, and a
one-field lookahead (`next_start`, `next_end`). The lookahead exists so the
iterator can answer `last()` and compare against `end()` without re-searching.

The constructor seeds the first field. Starting from a position,
it finds the next separator, and while `skip_blank` is on and the field is empty
(`start == end`), it keeps advancing past blanks. `next()` does
the same work for the lookahead field. When a scan reaches
`str.size()`, both bounds become `S::npos`, which is the exhausted state and
the value `end()` reports.

Fields are yielded as views, not copies. `operator*` constructs a `V` from the
source string and returns `substr(start, end - start)`, or `substr(start)` for
the final field where `end == S::npos`. With `V` being
`std::string_view`, the returned view aliases the original
buffer and no allocation happens per field.

Equality compares the owning split plus both bounds, so two
iterators match only when they sit at the same field of the same `Split`. The
`explicit operator bool()` reports whether the iterator is still positioned
(`start != npos`), and `last()` reports whether the lookahead has run out.

## skip_blank

`skip_blank` is a single flag consulted in both the constructor and `next()`
loops. When set, an empty field (the separator
sits where the field would start) is skipped over rather than yielded. When
clear, empty fields are emitted as zero-length views.

There is a naming wrinkle worth knowing: the constructor sets `skip_blank` to
`true` by default, and the `SKIP_BLANK_FIND` / `SKIP_BLANK_FIND_FIRST_OF` enum
values set it to `false`. So those two modes keep
blank fields. The static helpers take `skip_blank` as an ordinary boolean
parameter defaulting to `true`, with the intuitive meaning.

## Static helpers

`split` and `split_first_of` are a separate, eager code
path. They loop with `find` / `find_first_of`, and for each segment write a
substring of `S` (an owning copy, not a view) into the supplied output iterator,
honoring `skip_blank`. The trailing segment is emitted only when `prev <
str.size()`, which means a trailing empty field after a final
delimiter is never produced by the helpers. `split_first_of(char)` forwards to
`split(char)` because a single-character set and a single-character delimiter are
equivalent.

## Complexity

The iterator makes a single left-to-right pass over the string and performs no
allocation: each field is a `string_view` into `str`. Cost is dominated by the
underlying `find` / `find_first_of` scans, which together touch each character a
constant number of times. The static helpers are also single-pass but allocate
one `S` per emitted field (the substrings copied into the output iterator).

`Split::size()` is not free: it is `std::distance(begin(), end())`, which walks the whole string to count fields.

## Design decisions

- Member-function-pointer dispatch trades a tiny indirect-call cost for a
  branch-free search loop and one decision point at construction.
- The `Split` object owns a copy of the source string. That makes the object
  self-contained and safe to build from a temporary, at the cost of one copy of
  the input.
- The iterator carries a one-field lookahead so `last()` and end-comparison are
  O(1) and don't re-run the search.

## Limitations

- The yielded views alias the `Split`'s internal `str`, so they are valid only
  for the lifetime of the `Split` object.
- No regex, no locale awareness, no streaming over non-contiguous input.
- `size()` is O(n) in the string length.
- The `SKIP_BLANK_*` enum names are the opposite of what they do.
