// g++ -O3 -I ../include mod.cpp -I ../test -fopenmp
#include "constdiv.hpp"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <cybozu/option.hpp>

using namespace constdiv;

struct Mod2 {
	uint32_t M_ = 0; // max x
	uint32_t d_ = 0; // divisor
	uint32_t r_M_ = 0; // M % d
	uint32_t a_ = 0; // index of 2-power
	uint32_t a2_ = 0; // for mod
	uint32_t c_ = 0; // c = floor(A/d) = (A + d - 1) // d
	uint32_t c2_ = 0; // for mod
	uint32_t e_ = 0; // e = c d - A
	bool over_ = false; // c > M?
	bool cmp_ = false; // d > M_//2 ?
	void put() const
	{
		printf("d=0x%08x over_=%d a2=%u c2=0x%08x\n", d_, over_, a2_, c2_);
	}
	bool init(uint32_t d, uint32_t M = 0xffffffff)
	{
		if (d == 0 || d > M) return false;
		M_ = M;
		d_ = d;
		r_M_ = uint32_t(M % d);
		const uint32_t M_d = (r_M_ == d-1) ? M : M - r_M_ - 1;
		const uint32_t Mbit = bit_width(M);
		const uint32_t dbit = bit_width(d);
		if (is2power(d)) {
			a_ = dbit - 1;
			c_ = 1;
			return true;
		}
		if (d > M/2) {
			cmp_ = true;
			return true;
		}
		// c > 1 => A >= d => a >= ilog2(d)
		for (uint32_t a = dbit + 1; a < 64; a++) {
			uint64_t A = one << a;
			uint64_t c = (A + d - 1) / d;
			if (c >> (Mbit+1)) return false;
			uint64_t e = d * c - A;
			if (e > M) continue;
			if (e * M_d < A) {
				a_ = a;
				c_ = uint32_t(c);
				e_ = uint32_t(e);
				over_ = (c >> Mbit) != 0;

				for (uint32_t a2 = dbit + 1; a2 < 64; a2++) {
					uint64_t A2 = one << a2;
					uint64_t c2 = (A2 + d - 1) / d;
					if (c2 >> Mbit) continue;
					uint64_t e2 = d * c2 - A2;
					if (e2 * M_d / A2 < d + 1 && e2 * M / A2 < 2 * d - r_M_) {
						a2_ = a2;
						assert(c2 <= 0xffffffff);
						c2_ = uint32_t(c2);
						return true;
					}
				}
				return false;
			}
		}
		return false;
	}
	uint32_t umod(uint32_t x) const
	{
		if (cmp_) {
			if (x >= d_) x -= d_;
			return x;
		}
		if (c_ == 1) {
			return x & (d_ - 1);
		}
		if (over_) {
			int64_t v = int64_t((x * uint64_t(c2_)) >> a2_) * d_;
			v = x - v;
			if (v < 0) {
				v += d_;
			}
			return uint32_t(v);
		}
		int64_t v = int64_t((x * c_) >> a_) * d_;
		v = x - v;
		return uint32_t(v);
	}
	int smod(int x) const
	{
		assert(!cmp_ && c_ != 1);
		if (over_) {
			int64_t q = (x * int64_t(c2_)) >> a2_;
			int64_t v = x - q * d_;
			return int(v);
		}
		int64_t q = (x * int64_t(c_)) >> a_;
		int64_t v = x - q * d_;
		return int(v);
	}
};

struct Vd {
	uint32_t v = 0;
	uint32_t d = 0;
	void put(const char *msg = nullptr) const
	{
		if (msg) printf("%s ", msg);
		printf("0x%08x(d=0x%08x)", v, d);
	}
	void update_if_lt(const Vd& other)
	{
		if (other.v < v) {
			v = other.v;
			d = other.d;
		} else if (other.v == v) {
			if (other.d < d) {
				d = other.d; // smallest d
			}
		}
	}
	void update_if_gt(const Vd& other)
	{
		if (other.v > v) {
			v = other.v;
			d = other.d;
		} else if (other.v == v) {
			if (other.d < d) {
				d = other.d; // smallest d
			}
		}
	}
};
struct Stat {
	Vd maxc2{0, 0};
	Vd minc2{uint32_t(-1), 0};
	void combine(const Stat& other)
	{
		maxc2.update_if_gt(other.maxc2);
		minc2.update_if_lt(other.minc2);
	}
	void update(const Mod2& mod)
	{
		const Vd vc2{mod.c2_, mod.d_};
		maxc2.update_if_gt(vc2);
		minc2.update_if_lt(vc2);
	}
	void put() const
	{
		puts("Stat");
		maxc2.put("max c2="); minc2.put(" min c2="); puts("");
	}
};

void checkd(const Mod2& mod)
{
	const uint32_t M = mod.M_;
	const int H = M/2;
	const int d = mod.d_;
	const int min = -d/2;
	const int max = d + d/2;
	for (int x = -H; x <= H; x++) {
		int r = mod.smod(x);
		if (min < r && r < max && ((x - r) % d == 0)) continue;
		printf("ERR d=%d x=%d x%%d=%d r=%d\n", d, x, x % d, r);
	}
}

int main(int argc, char *argv[])
{
	cybozu::Option opt;
	uint32_t M;
	int d;
	bool showStat;
	opt.appendOpt(&M, 0xffffffff, "m", "max of x");
	opt.appendOpt(&d, 0, "d", "divisor");
	opt.appendBoolOpt(&showStat, "st", "show statistics");
	opt.appendHelp("h", "show this message");
	if (!opt.parse(argc, argv)) {
		opt.usage();
		return 1;
	}
	if (d) {
		printf("d=%u\n", d);
		Mod2 mod;
		if (mod.init(d, M)) {
			mod.put();
			checkd(mod);
			puts("ok");
		} else {
			puts("init err");
		}
		return 0;
	} else {
		printf("check all d in [1, 0x%08x]\n", M);
		const int H = M/2;
		printf("H=%d\n", H);
		#pragma omp parallel for
		for (int d = 1; d <= H; d++) {
//			if ((d & 1) == 0) continue;
			Mod2 mod;
			if (mod.init(d, M)) {
				if (mod.c_ == 1) continue;
//				if (mod.over_) continue;
				checkd(mod);
			}
		}
		puts("ok");
		return 0;
	}
	if (!showStat) return 0;
	Stat stat;
	#pragma omp declare reduction(range_red: Stat: omp_out.combine(omp_in)) initializer(omp_priv = Stat())
	#pragma omp parallel for reduction(range_red:stat)
	for (int d = 1; d <= 0x7fffffff; d++) {
		Mod2 mod;
		mod.init(d);
		if (mod.over_) {
			stat.update(mod);
		}
	}
	stat.put();
}
