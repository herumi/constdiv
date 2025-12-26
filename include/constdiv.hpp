#include <stdint.h>
#include <fstream>

//#include "uint128_t.hpp"

/*
M: integer >= 1.
d in [1, M]
take A >= d.
  c := (A + d - 1) // d.
  e := d c - A.
  M_d := M - ((M+1)%d).

Lemma.
1. By definition,
  0 <= e <= d-1 < A.
  M_d%d = d-1.
  d-1 <= M_d.
2. if e M_d < A, then e(d-1) < A = d c - e, so e < c.

Theorem
if e M_d < A, then x//d = (x c)//A for x in [0, M].

Proof
(q, r) := divmod(x, d). x = d q + r.
Then x c = d q c + r c = (A + e) q + r c = A q + (q e + r c).
y := q e + r c.
If 0 <= y < A, then q = (x c) // A.
So we prove that y < A if e M_d < A.
f(x) := dy = d q e + d r c = d q e + (A + e) r = (d q + r) e + r A = x e + r A.
So if max f(x) < d A, then max(y) = max f(x) / d < A.

e and A are constant values and >= 0, then arg_max f(x) = M_d or M.
i.e., (x, r) = (M_d, d-1) or (M, r0) where r0 := M % d.

Claim max f(x) = f(M_d) < d A.
case 1. M_d = M. then r0 = d-1. max f(x) = f(M_d) = M_d e + (d-1) A < A + (d-1) A = d A.
case 2. M_d < M. then r0 < d-1. max f(x) = max(f(M_d), f(M)).
M = M_d + 1 + r0.
f(M_d) - f(M) = (M_d e + (d-1)A) - (M e + r0 A) = (d-1 - r0) A - (r0 + 1)e
>(d-1 - (d-2)) A - ((d-2)+1)e = A - d e + e = d (c - e) > 0.
Then max f(x) = f(M_d) < d A.

This condition is the assumption of Thereom 1 in
"Integer division by constants: optimal bounds", Daniel Lemire, Colin Bartlett, Owen Kaser. 2021
*/

#if defined(_WIN64) || defined(__x86_64__)
#define CONST_DIV_GEN
#define CONST_DIV_GEN_X64

#define XBYAK_DISABLE_AVX512
#include <xbyak/xbyak_util.h>

#elif defined(__arm64__) || defined(__aarch64__)

#define CONST_DIV_GEN
#define CONST_DIV_GEN_AARCH64

#include <xbyak_aarch64/xbyak_aarch64.h>
#endif

namespace constdiv {

static const uint64_t one = 1;

static inline bool is2power(uint32_t x)
{
	return (x & (x-1)) == 0;
}

static inline uint32_t bit_width(uint64_t x)
{
	if (x == 0) return 0;
	uint32_t n = 0;
	while (x) {
		x >>= 1;
		n++;
	}
	return n;
}

static inline uint32_t ceil_ilog2(uint32_t x)
{
#if 1
	uint32_t a = bit_width(x);
	if (is2power(x)) return a - 1;
	return a;
#else
	assert(x > 0);
	uint32_t a = 0;
	for (;;) {
		if (x <= (one << a)) return a;
		a++;
	}
#endif
}

static inline uint64_t mask(uint32_t n)
{
	if (n == 64) return 0xffffffffffffffff;
	return (one << n) - 1;
}

struct ConstDivMod {
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
		printf("DivMod d=%d(0x%08x) a=%u c=0x%s%08x e=0x%08x cmp=%u over=%u\n", d_, d_, a_, over_ ? "1" : "", c_, e_, cmp_, over_);
		if (over_) {
			uint64_t v = ((M_*c2_)>>a2_)*d_;
			printf("mod a2=%u c=0x%08x tmp max=%" PRIx64 " tmp>M=%u\n", a2_, c2_, v, v>M_);
		}
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
				assert(c <= 0xffffffff);
				assert(e <= 0xffffffff);
				c_ = uint32_t(c);
				e_ = uint32_t(e);
				over_ = (c >> Mbit) != 0;

				// for mod
				for (uint32_t a2 = dbit + 1; a2 < 64; a2++) {
					uint64_t A2 = one << a2;
					uint64_t c2 = (A2 + d - 1) / d;
					if (c2 >> Mbit) continue;
					uint64_t e2 = d * c2 - A2;
					if (e2 * M_d / A2 < d + 1 && e2 * M / A2 < 2 * d - r_M_) {
#if 0 // enable if v <= M_ is expected
						uint64_t v = ((M*c2_)>>a2_)*d_;
						if (v > M_) {
							continue;
						}
#endif
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
	uint32_t divd(uint32_t x) const
	{
		if (cmp_) {
			return x >= d_;
		}
		if (c_ == 1) {
			return x >> a_;
		}
		if (over_) {
			uint64_t v = x * uint64_t(c_);
			v >>= 32;
			v += x;
			v >>= a_-32;
			return uint32_t(v);
		} else {
			uint32_t v = uint32_t((x * uint64_t(c_)) >> a_);
			return v;
		}
	}
	uint32_t modd(uint32_t x) const
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
};


typedef uint32_t (*FuncType)(uint32_t);

#ifdef CONST_DIV_GEN_X64

static const size_t DIV_FUNC_N = 4;
static const size_t MOD_FUNC_N = 4;

struct ConstDivModGen : ConstDivMod, Xbyak::CodeGenerator {
	FuncType divd = nullptr;
	FuncType divLp[DIV_FUNC_N] = {};
	const char *divName[DIV_FUNC_N] = {};
	static const int bestDivMode = 0;

	FuncType modd = nullptr;
	FuncType modLp[MOD_FUNC_N];
	const char *modName[MOD_FUNC_N] = {};
	static const int bestModMode = 1;
	ConstDivModGen()
		: Xbyak::CodeGenerator(4096, Xbyak::DontSetProtectRWE)
	{
	}
	bool init(uint32_t d, uint32_t lpN = 1)
	{
		using namespace Xbyak;
		using namespace Xbyak::util;

		if (!ConstDivMod::init(d)) return false;
		for (int j = 0; j < 2; j++) {
			auto f = j == 0 ? &ConstDivModGen::divRaw : &ConstDivModGen::modRaw;
			const uint32_t N = j == 0 ? DIV_FUNC_N : MOD_FUNC_N;
			int bestMode = j == 0 ? bestDivMode : bestModMode;
			auto funcPtr = j == 0 ? &divd : &modd;
			auto lpTbl = j == 0 ? divLp : modLp;
			{
				align(32);
				*funcPtr = getCurr<FuncType>();
				StackFrame sf(this, 1, UseRDX);
				const Reg32 x = sf.p[0].cvt32();
				(this->*f)(bestMode, x);
			}
			for (uint32_t mode = 0; mode < N; mode++) {
				align(32);
				lpTbl[mode] = getCurr<FuncType>();
				StackFrame sf(this, 1, 4|UseRDX);
				const Reg32 n = sf.p[0].cvt32();
				const Reg32 sum = sf.t[0].cvt32();
				const Reg32 x = sf.t[1].cvt32();
				const Reg32 t = sf.t[2].cvt32();
				const Reg32 i = sf.t[3].cvt32();
				xor_(sum, sum);
				xor_(i, i);
				align(32);
				Label lpL;
				L(lpL);
				mov(x, i);
				for (uint32_t j = 0; j < lpN; j++) {
					mov(t, x);
					(this->*f)(mode, t);
					add(sum, eax);
					add(x, sum);
				}
				add(i, 1);
				cmp(i, n);
				jb(lpL);
				mov(eax, sum);
			}
		}
		setProtectModeRE();
		return true;
	}
	void dump() const
	{
		std::ofstream ofs("dump.bin", std::ios::binary);
		ofs.write(reinterpret_cast<const char*>(getCode()), getSize());
	}
	// eax = x/d
	// use rax, rdx
	void divRaw(uint32_t mode, const Xbyak::Reg32& x)
	{
		if (cmp_) {
			divName[mode] = "cmp";
			xor_(eax, eax);
			cmp(x, d_);
			setae(al);
			return;
		}
		if (!over_) {
			if (c_ == 1) {
				divName[mode] = "shr";
				mov(eax, x);
				shr(rax, a_);
			} else {
				divName[mode] = "mul+shr";
#if 0
				mov(rdx, c_ << (64 - a_));
				mulx(rax, rdx, x.cvt64());
#else
				mov(eax, x);
				mov(edx, c_);
				mul(rdx);
				shr(rax, a_);
#endif
			}
			return;
		}
		assert(over_);
		uint64_t c33 = (one << 32) | c_;
		switch (mode) {
		case 0:
			divName[0] = "mul64";
			mov(rdx, c33 << (64 - a_));
			mulx(rax, rdx, x.cvt64());
			return;
		case 1:
			divName[1] = "mul32";
			mov(eax, c_);
			imul(rax, x.cvt64());
			shr(rax, 32);
			add(rax, x.cvt64());
			shr(rax, a_ - 32);
			return;
		case 2:
			divName[2] = "mul64+shrd";
			mov(rdx, c33);
			mulx(rdx, rax, x.cvt64());
			shrd(rax, rdx, a_);
			return;
		default:
			divName[DIV_FUNC_N-1] = "clang";
			// generated asm code by gcc/clang
			mov(edx, x);
			mov(eax, c_);
			imul(rax, rdx);
			shr(rax, 32);
			sub(edx, eax);
			shr(edx, 1);
			add(eax, edx);
			shr(eax, a_ - 33);
			return;
		}
	}
	// input: x
	// output: x = x * d
	void fast_muli(const Xbyak::Reg64& x, uint32_t d, const Xbyak::Reg64& t)
	{
		assert(d < 0x80000000);
		switch (d) {
		case 1:
			break;
		case 2:
			shl(x, 1);
			break;
		case 3:
			lea(x, ptr[x+x*2]);
			break;
		case 4:
			shl(x, 2);
			break;
		case 5:
			lea(x, ptr[x+x*4]);
			break;
		case 6:
			lea(x, ptr[x+x*2]);
			add(x, x);
			break;
		case 7:
			mov(t.cvt32(), x.cvt32());
			shl(x, 3);
			sub(x, t);
			break;
		case 8:
			shl(x, 3);
			break;
		case 9:
			lea(x, ptr[x+x*8]);
			break;
		default:
			if (d < 0x80000000) {
				imul(x, x, d);
			} else {
				mov(t.cvt32(), d);
				imul(x, x, t);
			}
			break;
		}
	}
	// input: x, eax = q
	// output: eax = q - d * x
	// destroy: edx
	void x_sub_qd(const Xbyak::Reg32& x)
	{
		fast_muli(rax, d_, rdx);
		mov(edx, eax);
		mov(eax, x.cvt32());
		sub(eax, edx);
	}
	// eax = x % d
	// use rax, rdx
	void modRaw(uint32_t mode, const Xbyak::Reg32& x)
	{
		const auto xq = x.cvt64();
		if (cmp_) {
			modName[mode] = "cmp";
			mov(eax, x);
			sub(eax, d_);
			cmovc(eax, x); // x < d ? x : x-d
			return;
		}
		if (!over_) {
			mov(eax, x);
			if (c_ == 1) {
				// d is 2-power
				modName[mode] = "and";
				mov(eax, x);
				and_(eax, d_ - 1);
			} else {
				modName[mode] = "mul+shr";
				mov(edx, c_);
				mul(rdx);
				shr(rax, a_);

				x_sub_qd(x);
			}
			return;
		}
		assert(over_);
		uint64_t c33 = (one << 32) | c_;
		switch (mode) {
		case 0:
			modName[0] = "mul64";
#if 0
			if (d == 95) {
				mov(rax, 194176253407468965);
				imul(xq, rax);
				mov(eax, dx);
				mul(xq);
				mov(eax, edx);
				break;
			}
#endif
			mov(rdx, c33 << (64 - a_));
			mulx(rax, rdx, xq);
			x_sub_qd(x);
			break;

		case 1:
			modName[1] = "mul32";
			mov(eax, c_);
			imul(rax, xq);
			shr(rax, 32);
			add(rax, xq);
			shr(rax, a_ - 32);
			x_sub_qd(x);
			break;

		case 2:
			modName[2] = "smallc";
#if 1
			mov(eax, x);
			if (is2power(c2_)) {
				uint32_t len = bit_width(c2_);
				shr(x, a2_ - (len - 1));
			} else {
				fast_muli(xq, c2_, rdx);
				shr(xq, a2_);
			}
			fast_muli(xq, d_, rdx);
			sub(rax, xq);
			lea(edx, ptr[eax + d_]);
			cmovc(eax, edx);
#else
			// slow
			mov(eax, x);
			fast_muli(xq, c2_, rdx);
			shr(xq, a2_);
			fast_muli(xq, d_, rdx);
			sub(rax, xq);
			sbb(edx, edx);
			and_(edx, d_);
			add(eax, edx);
#endif
			break;

		default:
			modName[3] = "clang";
			// generated asm code by gcc/clang
			mov(edx, x);
			mov(eax, c_);
			imul(rax, rdx);
			shr(rax, 32);
			sub(edx, eax);
			shr(edx, 1);
			add(eax, edx);
			shr(eax, a_ - 33);

			x_sub_qd(x);
			break;
		}
	}
};

#elif defined(CONST_DIV_GEN_AARCH64)

static const size_t DIV_FUNC_N = 4;
static const size_t MOD_FUNC_N = 4;

static inline Xbyak_aarch64::WReg cvt32(const Xbyak_aarch64::Reg& x)
{
	return Xbyak_aarch64::WReg(x.getIdx());
}

struct ConstDivModGen : Xbyak_aarch64::CodeGenerator, ConstDivMod {
	FuncType divd = nullptr;
	FuncType divLp[DIV_FUNC_N] = {};
	const char *divName[DIV_FUNC_N] = {};
	static const int bestDivMode = 0;

	FuncType modd = nullptr;
	FuncType modLp[MOD_FUNC_N];
	const char *modName[MOD_FUNC_N] = {};
	static const int bestModMode = 0;

	bool init(uint32_t d, uint32_t lpN = 1)
	{
		using namespace Xbyak_aarch64;
		if (!ConstDivMod::init(d)) return false;
		for (int j = 0; j < 2; j++) {
			auto f = j == 0 ? &ConstDivModGen::divRaw : &ConstDivModGen::modRaw;
			const uint32_t N = j == 0 ? DIV_FUNC_N : MOD_FUNC_N;
			int bestMode = j == 0 ? bestDivMode : bestModMode;
			auto funcPtr = j == 0 ? &divd : &modd;
			auto lpTbl = j == 0 ? divLp : modLp;
			{
				align(32);
				*funcPtr = getCurr<FuncType>();
				(this->*f)(cdm_, bestMode, x0);
				ret();
			}
			for (uint32_t mode = 0; mode < N; mode++) {
				align(32);
				lpTbl[mode] = getCurr<FuncType>();
				const auto n = w0;
				const auto sum = x1;
				const auto i = x2;
				const auto x = x3;
				const auto t = x4;
				mov(sum, 0);
				mov(i, 0);
				Label lpL;
				L(lpL);
				mov(x, i);
				for (uint32_t j = 0; j < lpN; j++) {
					mov(cvt32(t), cvt32(x));
					(this->*f)(cdm_, mode, t);
					add(sum, sum, t);
					add(x, x, sum);
				}
				add(i, i, 1);
				subs(n, n, 1);
				bne(lpL);
				mov(x0, sum);
				ret();
			}
		}
		ready();
		return true;
	}
	void dump() const
	{
		std::ofstream ofs("bin", std::ios::binary);
		ofs.write(reinterpret_cast<const char*>(getCode()), getSize());
	}
	// input x
	// output x = x/d
	// use w9, w10
	void divRaw(uint32_t mode, const Xbyak_aarch64::XReg& x)
	{
		using namespace Xbyak_aarch64;
		const auto wx = cvt32(x);
		if (cmp_) {
			divName[mode] = "cmp";
			mov_imm(w9, d_);
			cmp(x, x9);
			cset(x, HS); // Higher or Same
			return;
		}
		if (!over_) {
			if (c_ > 1) {
				mov_imm(w9, c_);
				divName[mode] = "mul+shr";
				umull(x, wx, w9);
			} else {
				divName[mode] = "shr";
			}
			lsr(x, x, a_);
			return;
		}
		switch (mode) {
		case 0:
			divName[0] = "mul64";
#if 1
			mov_imm(x9, uint64_t(c_) << (64 - a_));
			umulh(x, x9, x);
#else
			mov_imm(w9, c_);
			movk(x9, 1, 32); // x9 = c = [1:cH:cL]
			mul(x10, x9, x);
			umulh(x9, x9, x); // [x9:x10] = c * x
			extr(x, x9, x10, a_); // x = [x9:x10] >> a_
#endif
			return;
		case 1:
			divName[1] = "mul32";
			mov_imm(w9, c_);
			umull(x9, wx, w9); // x9 = [cH:cL] * x
			add(x, x, x9, LSR, 32); // x += x9 >> 32;
			lsr(x, x, a_ - 32);
			return;
		case 2:
			divName[2] = "mul64+extr";
			mov_imm(w9, c_);
			movk(x9, 1, 32); // x9 = c = [1:cH:cL]
			mul(x10, x9, x);
			umulh(x9, x9, x); // [x9:x10] = c * x
			extr(x, x9, x10, a_); // x = [x9:x10] >> a_
			return;
		default:
			divName[DIV_FUNC_N-1] = "clang";
			// same code generated by clang
			mov_imm(w9, c_);
			umull(x9, wx, w9);
			lsr(x9, x9, 32);
			sub(w10, wx, w9);
			add(w9, w9, w10, LSR, 1);
			lsr(wx, w9, a_ - 33);
			return;
		}
	}

	// x *= y, using t
	// return true without umull
	bool fast_muli(const Xbyak_aarch64::XReg& x, uint32_t y, const Xbyak_aarch64::XReg& t)
	{
		using namespace Xbyak_aarch64;
		if (x.getIdx() == t.getIdx()) {
			fprintf(stderr, "same regs are not allowed\n");
			exit(1);
		}
		const auto wx = cvt32(x);
		const auto wt = cvt32(t);
		switch (y) {
		case 1:
			break;
		case 2:
			lsl(x, x, 1);
			break;
		case 3:
			add(x, x, x, LSL, 1);
			break;
		case 4:
			lsl(x, x, 2);
			break;
		case 5:
			add(x, x, x, LSL, 2);
			break;
		case 6:
			add(x, x, x, LSL, 1);
			lsl(x, x, 1);
			break;
		case 7:
			lsl(t, x, 3);
			sub(x, t, x);
			break;
		case 8:
			lsl(x, x, 3);
			break;
		case 9:
			add(x, x, x, LSL, 3);
			break;
		case 10:
			add(x, x, x, LSL, 2);
			lsl(x, x, 1);
			break;
		default:
			mov_imm(wt, y);
			umull(x, wx, wt);
			return false;
		}
		return true;
	}

	// input:  x, q
	// output: x -= q * d
	// destroy: q
	void x_sub_qd(const Xbyak_aarch64::XReg& x, const Xbyak_aarch64::XReg& q, uint32_t d, const Xbyak_aarch64::XReg& t)
	{
		fast_muli(q, d, t); // t = x * d
		sub(x, x, q);
	}
	// input x
	// output x = x % d
	// use w9, w10
	void modRaw(uint32_t mode, const Xbyak_aarch64::XReg& x)
	{
		using namespace Xbyak_aarch64;
		const auto wx = cvt32(x);
		if (cmp_) {
			modName[mode] = "cmp";
			mov_imm(w9, d_);
			cmp(x, x9);
			csel(x10, xzr, x9, LT); // x10 = LT(cmp(x, x9)) ? 0 : x9
			sub(x, x, x10);
			return;
		}
		if (!over_) {
			if (c_ == 1) {
				modName[mode] = "shr";
				and_imm(wx, wx, d_-1, w9);
			} else {
				mov_imm(w9, c_);
				modName[mode] = "mul+shr";
				umull(x10, wx, w9);
				lsr(x10, x10, a_);
				x_sub_qd(x, x10, d_, x9);
			}
			return;
		}
		assret(over_);
		switch (mode) {
		case 0:
			modName[0] = "mul64";
#if 1
			mov_imm(x9, c_);
			lsl(x10, x9, 64 - a_);
			umulh(x10, x10, x); // [x9:x10] = c * x
			x_sub_qd(x, x10, d, x9);
#else
			mov_imm(w9, c_);
			movk(x9, 1, 32); // x9 = c = [1:cH:cL]
			mul(x10, x9, x);
			umulh(x9, x9, x); // [x9:x10] = c * x
			extr(x10, x9, x10, a_); // x = [x9:x10] >> a_
			x_sub_qd(x, x10, d, x9);
#endif
			return;
		case 1:
			modName[1] = "mul32";
			mov_imm(w9, c_);
			umull(x9, wx, w9); // x9 = [cH:cL] * x
			add(x10, x, x9, LSR, 32); // x += x9 >> 32;
			lsr(x10, x10, a_ - 32);
			x_sub_qd(x, x10, d, x9);
			return;
		case 2:
			modName[2] = "smallc";
			mov(x10, x);
			fast_muli(x10, c2_, x9);
			lsr(x10, x10, a2_);
#if 0
if (d == 3) { // same to msub
	mov(x9, d);
	sub(x, x, x10, LSL, 1); // x -= q * 2
	sub(x, x, x10);
} else {
			// msub(a, b, c, d)
			// a = d - b * c
			mov(x9, d);
			msub(x, x10, x9, x);
}
#else
			if (fast_muli(x10, d, x9)) {
				mov_imm(w9, d);
			}
			sub(x, x, x10); // x - q * d
#endif
			add(x9, x, x9); // x - q * d + d
			cmp(x, 0);
			csel(x, x, x9, GE); // x = (x >= 0) ? x - q d : x - q d + d
			return;
		default:
			modName[3] = "clang";
			// same code generated by clang
			mov_imm(w9, c_);
			umull(x9, wx, w9);
			lsr(x9, x9, 32);
			sub(w10, wx, w9);
			add(w9, w9, w10, LSR, 1);
			lsr(w10, w9, a_ - 33);
			x_sub_qd(x, x10, d_, x9);
			return;
		}
	}
};

#endif

} // constdiv

