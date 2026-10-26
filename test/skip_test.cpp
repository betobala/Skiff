#include "../src/Skip.hpp"
#include <cstdio>
using namespace skiff;
int main() {
	int fails = 0;
	Random rng;
	float ps[5] = {0.f, 0.25f, 0.5f, 0.75f, 1.f};
	for (int m = 0; m < 2; m++) {
		for (float p : ps) {
			Decider d; int pass = 0, runs = 0, longest = 0, run = 0; bool prev = false;
			for (int i = 0; i < 20000; i++) {
				bool b = d.decide(p, m == 1, rng);
				pass += b;
				run = (b == prev) ? run + 1 : 1; prev = b; longest = std::max(longest, run);
			}
			float ratio = pass / 20000.f;
			printf("%s p=%.2f -> %.3f, longest run %d\n", m ? "smooth" : "random", p, ratio, longest);
			if (std::fabs(ratio - p) > (m ? 0.03f : 0.02f)) fails++;
			if (!m && (p == 0.f || p == 1.f) && ratio != p) fails++;
		}
	}
	printf("cv: knob 1, 2.5 V -> %.2f; 6 V -> %.2f; unpatched knob .3 -> %.2f\n", probability(1, true, 2.5f), probability(1, true, 6), probability(0.3f, false, 0));
	printf(fails ? "FAILED\n" : "ALL OK\n");
	return fails;
}
