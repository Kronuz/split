// split comparison benchmark (-DSPLIT_BUILD_BENCH=ON).
//
// Each row times split, one comparator and split again; the spread of the two split runs is the
// noise of the row. Both sides produce the same fields, checked before the row is timed. The
// comparators are a memchr loop and string_view::find loops written here, and abseil's
// StrSplit when CMake finds it (HAVE_ABSL).
//
//   split_compare            all rows
//   split_compare iterator   only rows whose group or workload contains "iterator"
//   split_compare --check    the result checks only, no timing (CTest split_compare)
//
// Times are the best of seven samples of at least 8 ms, in ns per call.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "split.h"
#ifdef HAVE_ABSL
#include "absl/strings/str_split.h"
#endif

namespace {

using clk = std::chrono::steady_clock;
using Fields = std::vector<std::string_view>;

bool g_check = false;
const char* g_filter = nullptr;
int g_bad = 0;

template <class T>
inline void keep(const T& v) { asm volatile("" : : "r,m"(v) : "memory"); }

template <class F>
double measure(F&& f) {
	std::size_t it = 1;
	for (;;) {
		const auto t0 = clk::now();
		for (std::size_t i = 0; i < it; ++i) { f(); }
		if (std::chrono::duration<double>(clk::now() - t0).count() > 0.008) { break; }
		it *= 2;
	}
	double best = 1e30;
	for (int k = 0; k < 7; ++k) {
		const auto t0 = clk::now();
		for (std::size_t i = 0; i < it; ++i) { f(); }
		best = std::min(best, std::chrono::duration<double>(clk::now() - t0).count() / static_cast<double>(it));
	}
	return best * 1e9;
}

template <class A, class B>
void row(const char* group, const std::string& what, const char* comparator, bool same, A&& ours, B&& comp) {
	if (!same) {
		++g_bad;
		std::printf("MISMATCH %s: %s against %s\n", group, what.c_str(), comparator);
	}
	if (g_check) { return; }
	if (g_filter && !std::strstr(group, g_filter) && !std::strstr(what.c_str(), g_filter)) { return; }
	const double o1 = measure(ours);
	const double c = measure(comp);
	const double o2 = measure(ours);
	const double o = std::min(o1, o2);
	std::printf("%-9s %-36s %-28s %10.1f %10.1f  %6.2f  %5.1f%%\n", group, what.c_str(), comparator, o, c, o / c,
		100.0 * std::abs(o1 - o2) / o);
	std::fflush(stdout);
}

std::uint64_t rs = 88172645463325252ull;
inline std::uint64_t rnd() {
	rs ^= rs << 13;
	rs ^= rs >> 7;
	rs ^= rs << 17;
	return rs;
}

// Fields of 1 to 15 characters, one in ten empty, joined by `sep`; the last field is not empty.
std::string make_fields(std::size_t n, std::string_view sep, bool second_sep = false) {
	std::string s;
	while (s.size() < n) {
		if (rnd() % 10 != 0) {
			const std::size_t len = 1 + rnd() % 15;
			for (std::size_t i = 0; i < len; ++i) { s.push_back("abcdefghijklmnopqrstuvwxyz0123456789"[rnd() % 36]); }
		}
		if (second_sep && (rnd() & 1)) { s.push_back(';'); } else { s.append(sep); }
	}
	s.append("end");
	return s;
}

// The comparator every other splitter is measured against: memchr and a vector.
Fields memchr_split(std::string_view sv, char sep, bool skip_blank) {
	Fields v;
	const char* p = sv.data();
	const char* const e = p + sv.size();
	while (p < e) {
		const char* q = static_cast<const char*>(std::memchr(p, sep, static_cast<std::size_t>(e - p)));
		if (!q) { q = e; }
		if (!skip_blank || q != p) { v.emplace_back(p, static_cast<std::size_t>(q - p)); }
		p = q + 1;
	}
	return v;
}

// The same walk without a vector: the sum of the field sizes, the comparator of the iterator rows.
std::size_t memchr_sum(std::string_view sv, char sep) {
	std::size_t k = 0;
	const char* p = sv.data();
	const char* const e = p + sv.size();
	while (p < e) {
		const char* q = static_cast<const char*>(std::memchr(p, sep, static_cast<std::size_t>(e - p)));
		if (!q) { q = e; }
		k += static_cast<std::size_t>(q - p);
		p = q + 1;
	}
	return k;
}

Fields find_split(std::string_view sv, std::string_view sep) {
	Fields v;
	std::size_t prev = 0;
	for (std::size_t next; (next = sv.find(sep, prev)) != std::string_view::npos; prev = next + sep.size()) {
		if (next != prev) { v.push_back(sv.substr(prev, next - prev)); }
	}
	if (prev < sv.size()) { v.push_back(sv.substr(prev)); }
	return v;
}

void run() {
	for (const std::size_t n : {64, 1024, 65536}) {
		const std::string tag = ", " + std::to_string(n) + " B";

		// One character, empty fields dropped, into a vector.
		const std::string csv = make_fields(n, ",");
		const std::string_view sv = csv;
		const auto split_vec = [&] {
			Fields v;
			Split<std::string_view>::split(sv, ',', std::back_inserter(v));
			return v;
		};
		const Fields want = memchr_split(sv, ',', true);
		row("split()", "char" + tag, "memchr loop", split_vec() == want, [&] { keep(split_vec()); },
			[&] { keep(memchr_split(sv, ',', true)); });
		row("split()", "char" + tag, "string_view::find loop", split_vec() == find_split(sv, ","), [&] { keep(split_vec()); },
			[&] { keep(find_split(sv, ",")); });

		// One character, keeping the empty fields.
		const auto split_blank = [&] {
			Fields v;
			Split<std::string_view>::split(sv, ',', std::back_inserter(v), false);
			return v;
		};
		row("split()", "char, keep blanks" + tag, "memchr loop", split_blank() == memchr_split(sv, ',', false),
			[&] { keep(split_blank()); }, [&] { keep(memchr_split(sv, ',', false)); });

		// The iterator, summing the field sizes.
		const auto iterate = [&] {
			std::size_t k = 0;
			Split<std::string_view> sp(sv, ',');
			for (const auto f : sp) { k += f.size(); }
			return k;
		};
		row("iterator", "Split<string_view>" + tag, "memchr loop, no vector", iterate() == memchr_sum(sv, ','),
			[&] { keep(iterate()); }, [&] { keep(memchr_sum(sv, ',')); });
		const auto iterate_owning = [&] {
			std::size_t k = 0;
			Split<std::string> sp(csv, ',');
			for (const auto f : sp) { k += f.size(); }
			return k;
		};
		row("iterator", "Split<string> (copies the source)" + tag, "memchr loop, no vector", iterate_owning() == memchr_sum(sv, ','),
			[&] { keep(iterate_owning()); }, [&] { keep(memchr_sum(sv, ',')); });

		// A delimiter of two characters.
		const std::string colons = make_fields(n, "::");
		const std::string_view cv = colons;
		const std::string_view two = "::";
		const auto split_two = [&] {
			Fields v;
			Split<std::string_view>::split(cv, two, std::back_inserter(v));
			return v;
		};
		row("split()", "\"::\"" + tag, "string_view::find loop", split_two() == find_split(cv, two), [&] { keep(split_two()); },
			[&] { keep(find_split(cv, two)); });

		// Any of two characters.
		const std::string mixed = make_fields(n, ",", true);
		const std::string_view mv = mixed;
		const std::string_view any = ",;";
		const auto split_any = [&] {
			Fields v;
			Split<std::string_view>::split_first_of(mv, any, std::back_inserter(v));
			return v;
		};
		const auto hand_any = [&] {
			Fields v;
			std::size_t prev = 0;
			for (std::size_t next; (next = mv.find_first_of(any, prev)) != std::string_view::npos; prev = next + 1) {
				if (next != prev) { v.push_back(mv.substr(prev, next - prev)); }
			}
			if (prev < mv.size()) { v.push_back(mv.substr(prev)); }
			return v;
		};
		row("split()", "first_of \",;\"" + tag, "find_first_of loop", split_any() == hand_any(), [&] { keep(split_any()); },
			[&] { keep(hand_any()); });

#ifdef HAVE_ABSL
		const auto absl_vec = [&] { return Fields(absl::StrSplit(sv, ',', absl::SkipEmpty())); };
		row("split()", "char" + tag, "absl::StrSplit", split_vec() == absl_vec(), [&] { keep(split_vec()); },
			[&] { keep(absl_vec()); });
		const auto absl_blank = [&] { return Fields(absl::StrSplit(sv, ',')); };
		row("split()", "char, keep blanks" + tag, "absl::StrSplit", split_blank() == absl_blank(), [&] { keep(split_blank()); },
			[&] { keep(absl_blank()); });
		const auto absl_iter = [&] {
			std::size_t k = 0;
			for (const auto f : absl::StrSplit(sv, ',', absl::SkipEmpty())) { k += f.size(); }
			return k;
		};
		row("iterator", "Split<string_view>" + tag, "absl::StrSplit", iterate() == absl_iter(), [&] { keep(iterate()); },
			[&] { keep(absl_iter()); });
		row("iterator", "Split<string> (copies the source)" + tag, "absl::StrSplit", iterate_owning() == absl_iter(),
			[&] { keep(iterate_owning()); }, [&] { keep(absl_iter()); });
		const auto absl_two = [&] { return Fields(absl::StrSplit(cv, "::", absl::SkipEmpty())); };
		row("split()", "\"::\"" + tag, "absl::StrSplit", split_two() == absl_two(), [&] { keep(split_two()); },
			[&] { keep(absl_two()); });
		const auto absl_any = [&] { return Fields(absl::StrSplit(mv, absl::ByAnyChar(",;"), absl::SkipEmpty())); };
		row("split()", "first_of \",;\"" + tag, "absl::ByAnyChar", split_any() == absl_any(), [&] { keep(split_any()); },
			[&] { keep(absl_any()); });
#endif
	}
}

}  // namespace

int main(int argc, char** argv) {
	for (int i = 1; i < argc; ++i) {
		if (std::strcmp(argv[i], "--check") == 0) { g_check = true; }
		else { g_filter = argv[i]; }
	}
	if (!g_check) {
#ifdef HAVE_ABSL
		std::printf("# abseil comparators: yes\n");
#else
		std::printf("# abseil comparators: no\n");
#endif
		std::printf("%-9s %-36s %-28s %10s %10s  %6s  %6s\n", "group", "workload", "comparator", "ours ns", "comp ns", "ratio", "A/A");
	}
	run();
	if (g_bad) {
		std::fprintf(stderr, "%d rows disagree with their comparator\n", g_bad);
		return 1;
	}
	if (g_check) { std::printf("all split comparison checks passed\n"); }
	return 0;
}
