// Smoke test for the standalone split library.
// Build: c++ -std=c++17 -I.. test.cc -o test && ./test
#include <cassert>
#include <cstdio>
#include <iterator>
#include <string>
#include <vector>
#include "split.h"

int main() {
	// Static helper: split a string on a single char.
	std::vector<std::string> out;
	Split<>::split(std::string("alpha,beta,gamma"), ',', std::back_inserter(out));
	assert(out.size() == 3);
	assert(out[0] == "alpha" && out[1] == "beta" && out[2] == "gamma");

	// skip_blank (default true) drops empty fields.
	std::vector<std::string> out2;
	Split<>::split(std::string("a,,b,"), ',', std::back_inserter(out2));
	assert(out2.size() == 2 && out2[0] == "a" && out2[1] == "b");

	// Iterator form over a Split object.
	std::vector<std::string> out3;
	Split<> sp(std::string("x/y/z"), '/');
	for (const auto& part : sp) out3.push_back(std::string(part));
	assert(out3.size() == 3 && out3[0] == "x" && out3[2] == "z");

	std::printf("split OK: %zu + %zu + %zu parts\n",
	            out.size(), out2.size(), out3.size());
	return 0;
}
