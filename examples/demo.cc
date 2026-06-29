// A runnable tour of split.
//
// Build (when this repo is the top-level project):
//   cmake -B build && cmake --build build && ./build/split_demo
//
// split breaks a string into fields. The iterator form walks the string once and
// yields each field lazily as a std::string_view into the original buffer (no
// copies); the static helpers write fields straight into a container. This demo
// shows the lazy iterator, the container-filling helpers, the two delimiter modes
// (a full multi-char delimiter via FIND vs. any-character-in-a-set via
// FIND_FIRST_OF), and the skip_blank flag that drops or keeps empty fields.
#include <cstdio>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "split.h"

static void rule(const char* title) {
	std::printf("\n\033[1m── %s ──\033[0m\n", title);
}

// Print a vector of fields as quoted, comma-separated cells, so empty fields are
// visible as "" instead of disappearing.
static void show(const std::vector<std::string>& fields) {
	std::fputs("  [ ", stdout);
	for (std::size_t i = 0; i < fields.size(); ++i) {
		std::printf("\"%s\"%s", fields[i].c_str(), i + 1 < fields.size() ? ", " : " ");
	}
	std::printf("]   (%zu fields)\n", fields.size());
}

int main() {
	std::puts("split demo  (one pass over the string, fields without copies)");

	// --- 1. the lazy iterator yields string_views into the source ------------
	rule("range-for over a Split: each field is a view into the original");
	// Bind the Split to a named variable so its copy of the string outlives the
	// loop (the views alias that copy).
	Split<> sp(std::string("alpha/beta/gamma/delta"), '/');
	std::printf("  input  : \"alpha/beta/gamma/delta\"  (delimiter '/')\n");
	std::fputs("  fields :", stdout);
	for (std::string_view field : sp) {
		std::printf(" [%.*s]", (int)field.size(), field.data());
	}
	std::putc('\n', stdout);
	std::puts("  (each [..] is a string_view aliasing the Split's own buffer, no copy)");

	// --- 2. the static helper fills a container in one call ------------------
	rule("the static helper writes fields straight into a container");
	std::vector<std::string> out;
	Split<>::split(std::string("one,two,three"), ',', std::back_inserter(out));
	std::printf("  split(\"one,two,three\", ',') ->\n");
	show(out);

	// --- 3. full-delimiter (FIND) vs. any-char-in-set (FIND_FIRST_OF) ---------
	rule("FIND matches the whole delimiter; FIND_FIRST_OF matches any char in it");
	std::vector<std::string> by_string;
	Split<>::split(std::string("a::b::c"), std::string("::"), std::back_inserter(by_string));
	std::printf("  split(\"a::b::c\", \"::\")           full \"::\" delimiter ->\n");
	show(by_string);

	// The same "::" given to FIND_FIRST_OF is a SET {':' , ':'} = just ':', so a
	// single colon splits and the field between the two colons comes out empty.
	std::vector<std::string> by_set;
	Split<>::split_first_of(std::string("a::b::c"), std::string("::"),
		std::back_inserter(by_set), /*skip_blank=*/false);
	std::printf("  split_first_of(\"a::b::c\", \"::\")  any of {':' } ->\n");
	show(by_set);

	// A genuine multi-character set: split on a comma OR a semicolon.
	std::vector<std::string> mixed;
	Split<>::split_first_of(std::string("a,b;c,d"), std::string(",;"), std::back_inserter(mixed));
	std::printf("  split_first_of(\"a,b;c,d\", \",;\")  any of {',' ';'} ->\n");
	show(mixed);

	// --- 4. skip_blank: drop empty fields, or keep them ----------------------
	rule("skip_blank (default true) drops empty fields; pass false to keep them");
	std::vector<std::string> dropped, kept;
	Split<>::split(std::string("a,,b,"), ',', std::back_inserter(dropped));            // default true
	Split<>::split(std::string("a,,b,"), ',', std::back_inserter(kept), /*skip_blank=*/false);
	std::printf("  input \"a,,b,\"  (note the empty middle field and trailing comma)\n");
	std::fputs("  skip_blank=true  ->", stdout);  show(dropped);
	std::fputs("  skip_blank=false ->", stdout);  show(kept);
	std::puts("  (the static helper never emits the trailing empty after a final delimiter)");

	// --- 5. inspecting the iterator: last() flags the final field ------------
	rule("the iterator exposes last() so you can treat the final field specially");
	Split<> path(std::string("usr/local/bin"), '/');
	std::printf("  joining \"usr/local/bin\" with arrows: ");
	for (auto it = path.begin(); it != path.end(); ++it) {
		std::string_view field = *it;
		std::printf("%.*s%s", (int)field.size(), field.data(), it.last() ? "" : " -> ");
	}
	std::putc('\n', stdout);

	std::puts("\ndone.");
	return 0;
}
