#pragma once
// Dual probability skipper, written from the published behaviour of the Ladik S-090: each gate
// at the input passes to the output with a probability, set by the knob or by CV attenuated by
// the knob. Two jumpers become options: exclusive mode and the randomizing algorithm.
// Independent of Rack so it can be unit tested.
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace skiff {

/** knob 0..1; with CV patched the knob attenuates it: 0 V or less = 0 %, 5 V or more = 100 % */
inline float probability(float knob, bool cvPatched, float cvVolts) {
	float p = cvPatched ? knob * cvVolts / 5.f : knob;
	return std::max(0.f, std::min(1.f, p));
}

struct Random {
	uint32_t state = 0x9E3779B9u;
	float next() {
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return (state >> 8) / 16777216.f;
	}
};

/**
 * Decides whether one pulse passes.
 * smooth = false: independent coin flips (the lower jumper removed: 0 % and 100 % are exact,
 * clusters happen as they would by chance).
 * smooth = true: the jumper set: a running error term pulls the long-run ratio towards p and
 * breaks up clusters, so the randomness is more regular, while 0 % and 100 % are not reliable.
 */
struct Decider {
	float error = 0.f;

	bool decide(float p, bool smooth, Random& rng) {
		if (!smooth)
			return rng.next() < p;
		// jittered error diffusion: mostly random, nudged by how far behind or ahead we are
		float q = std::max(0.02f, std::min(0.98f, p + 0.6f * error));
		bool pass = rng.next() < q;
		error += p - (pass ? 1.f : 0.f);
		error = std::max(-2.f, std::min(2.f, error));
		return pass;
	}
};

}  // namespace skiff
