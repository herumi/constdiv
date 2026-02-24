// g++ -O3 -I ../include mod.cpp -I ../test -I ../ext/xbyak -fopenmp
#include "constdiv.hpp"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

using namespace constdiv;

struct Mod2 {
	uint32_t M_ = 0; // max x
	uint32_t d_ = 0; // divisor
	uint32_t r_M_ = 0; // M % d
	uint32_t a_ = 0; // index of 2-power
	uint32_t a2_ = 0; // for mod
	uint32_t a3_ = 0; // for mod
	uint32_t c_ = 0; // c = floor(A/d) = (A + d - 1) // d
	uint32_t c2_ = 0; // for mod
	uint32_t c3_ = 0; // for mod
	uint32_t e_ = 0; // e = c d - A
	bool over_ = false; // c > M?
	bool cmp_ = false; // d > M_//2 ?
	void put() const
	{
		printf("d=0x%08x over_=%d a2=%u c2=0x%08x a3=%u c3=0x%08x\n", d_, over_, a2_, c2_, a3_, c3_);
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

				// for mod
				bool found2 = false;
				bool found3 = false;
				for (uint32_t a2 = dbit + 1; a2 < 64; a2++) {
					uint64_t A2 = one << a2;
					uint64_t c2 = (A2 + d - 1) / d;
					if (c2 >> Mbit) continue;
					uint64_t e2 = d * c2 - A2;
					if (!found2 && (e2 * M_d / A2 < d + 1 && e2 * M / A2 < 2 * d - r_M_)) {
						a2_ = a2;
						assert(c2 <= 0xffffffff);
						c2_ = uint32_t(c2);
						found2 = true;
					}
					if (!found3 && (e2 * M / A2 < d + 1)) {
						a3_ = a2;
						assert(a3_ <= 0xffffffff);
						c3_ = uint32_t(c2);
						found3 = true;
					}
					if (found2 && found3) {
//						if (a2_ != a3_) put();
						return true;
					}
				}
				return false;
			}
		}
		return false;
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
	Vd maxc3{0, 0};
	Vd minc3{uint32_t(-1), 0};
	uint32_t diffN = 0;
	void combine(const Stat& other)
	{
		maxc2.update_if_gt(other.maxc2);
		minc2.update_if_lt(other.minc2);

		maxc3.update_if_gt(other.maxc3);
		minc3.update_if_lt(other.minc3);
		diffN += other.diffN;
	}
	void update(const Mod2& mod)
	{
		const Vd vc2{mod.c2_, mod.d_};
		maxc2.update_if_gt(vc2);
		minc2.update_if_lt(vc2);
		const Vd vc3{mod.c3_, mod.d_};
		maxc3.update_if_gt(vc3);
		minc3.update_if_lt(vc3);
		if (mod.a2_ != mod.a3_) diffN++;
	}
	void put() const
	{
		puts("Stat");
		maxc2.put("max c2="); minc2.put(" min c2="); puts("");
		maxc3.put("max c3="); minc3.put(" min c3="); puts("");
		printf("diffN=%u\n", diffN);
	}
};

int main(int argc, char *argv[])
{
	if (argc == 2) {
		uint32_t d = atoi(argv[1]);
		printf("d=%u\n", d);
		Mod2 mod;
		if (mod.init(d)) {
			mod.put();
			puts("ok");
		} else {
			puts("init err");
		}
		return 0;
	}
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
