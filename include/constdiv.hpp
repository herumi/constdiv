#include <stdint.h>
#include <fstream>
#include <print>
#include <bit>

#include "uint128_t.hpp"

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

static inline constexpr uint32_t ceil_ilog2(uint32_t x)
{
#if 1
	uint32_t a = std::bit_width(x);
	if ((x & (x-1)) == 0) {
		return a - 1;
	}
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

static inline constexpr uint64_t mask(uint32_t n)
{
	if (n == 64) return 0xffffffffffffffff;
	return (one << n) - 1;
}

struct ConstDivMod {
	uint32_t d_ = 0; // divisor
	uint32_t r_M_ = 0; // M % d
	uint32_t a_ = 0; // index of 2-power
	uint32_t a2_ = 0; // for mod
	bool cmp_ = false; // d > M_//2 ?
	bool over_ = false; // c > (M+1) ?
	uint64_t c_ = 0; // c = floor(A/d) = (A + d - 1) // d
	uint64_t c2_ = 0; // for mod
	uint64_t e_ = 0; // e = c d - A
	void put() const
	{
		std::print("DivMod d={}(0x{:08x}) a={} c=0x{:x} c32={} e=0x{:x} cmp={} over={}\n", d_, d_, a_, c_, (c_ >> 32) != 0, e_, cmp_, over_);
		if (over_) {
			std::print("mod a2={} c=0x{:x}\n", a2_, c2_);
		}
	}
	bool init(uint32_t d, uint64_t M = 0xffffffff)
	{
		if (d == 0 || d > M) return false;
		d_ = d;
		r_M_ = uint32_t(M % d);
		const uint32_t M_d = (r_M_ == d-1) ? M : M - r_M_ - 1;
		const uint32_t Mbit = std::bit_width(M);
		const uint32_t dbit = std::bit_width(d);
		if ((d & (d-1)) == 0) { // d is 2-power
			a_ = dbit - 1;
			c_ = 1;
			return true;
		}
		if (d > M/2) {
			cmp_ = true;
			return true;
		}
		// c > 1 => A >= d => a >= ilog2(d)
		for (uint32_t a = dbit + 1; a < 128; a++) {
			uint128_t A = uint128_t(one) << a;
			uint128_t c = (A + d - 1) / d;
			if (c >> (Mbit+1)) return false;
			uint64_t e = d * c - A;
			if (e * M_d < A) {
				a_ = a;
				c_ = uint64_t(c);
				e_ = e;
				over_ = (c >> Mbit) != 0;

				// for mod
				for (uint32_t a2 = dbit + 1; a2 < 128; a2++) {
					uint128_t A = uint128_t(one) << a2;
					uint128_t c = (A + d - 1) / d;
					if (c >> (Mbit+1)) return false;
					uint64_t e = d * c - A;
					if (e * M_d / A < d + 1 && e * M / A < 2 * d - r_M_) {
						if (c >> Mbit) {
							continue;
						}
						a2_ = a2;
						c2_ = c;
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
			uint64_t v = x * (c_ & 0xffffffff);
			v >>= 32;
			v += x;
			v >>= a_-32;
			return uint32_t(v);
		} else {
			uint32_t v = uint32_t((x * c_) >> a_);
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
			int64_t v = int64_t((x * c2_) >> a2_) * d_;
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

static const size_t DIV_FUNC_N = 2;
static const size_t MOD_FUNC_N = 3;

struct ConstDivModGen : Xbyak::CodeGenerator {
	FuncType divd = nullptr;
	FuncType divLp[DIV_FUNC_N] = {};
	const char *divName[DIV_FUNC_N] = {};
	static const int bestDivMode = 0;
	uint32_t d_ = 0;
	uint32_t a_ = 0;

	FuncType modd = nullptr;
	FuncType modLp[MOD_FUNC_N];
	const char *modName[MOD_FUNC_N] = {};
	static const int bestModMode = 0;
	ConstDivModGen()
		: Xbyak::CodeGenerator(4096, Xbyak::DontSetProtectRWE)
	{
	}
	// eax = x/d
	// use rax, rdx
	void divRaw(const ConstDivMod& cdm, uint32_t mode, const Xbyak::Reg32& x)
	{
		if (d_ >= 0x80000000) {
			divName[mode] = "cmp";
			xor_(eax, eax);
			cmp(x, d_);
			setae(al);
			return;
		}
		if (cdm.c_ <= 0xffffffff) {
			mov(eax, x);
			if (cdm.c_ > 1) {
				divName[mode] = "mul+shr";
				mov(edx, cdm.c_);
				mul(rdx);
			} else {
				divName[mode] = "shr";
			}
			shr(rax, cdm.a_);
			return;
		}
		if (mode == DIV_FUNC_N-1) {
			divName[DIV_FUNC_N-1] = "gcc";
			// generated asm code by gcc/clang
			mov(edx, x);
			mov(eax, cdm.c_ & 0xffffffff);
			imul(rax, rdx);
			shr(rax, 32);
			sub(edx, eax);
			shr(edx, 1);
			add(eax, edx);
			shr(eax, cdm.a_ - 33);
			return;
		}
		if (mode == bestDivMode) {
			divName[bestDivMode] = "my";
			mov(eax, cdm.c_ & 0xffffffff);
			imul(rax, x.cvt64());
			shr(rax, 32);
			add(rax, x.cvt64());
			shr(rax, cdm.a_ - 32);
			return;
		}
#if 0
		/*
			mul and mulx are almost same
			shrd is slow
		*/
		mov(eax, x);
		mov(rdx, cdm.c_);
		static const char *nameTbl[] = {
			"mul/mixed",
			"mulx/mixed",
			"mul/shrd",
			"mulx/shrd",
		};
		divName[mode] = nameTbl[mode];
		if (mode & (1<<0)) {
			mulx(rdx, rax, rax);
		} else {
			mul(rdx);
		}
		if (mode & (1<<1)) {
			shrd(rax, rdx, uint8_t(cdm.a_));
		} else {
			shr(rax, cdm.a_);
			shl(edx, 64 - cdm.a_);
			or_(eax, edx);
		}
#endif
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
			shl(x, 2);
			break;
		case 7:
			mov(t.cvt32(), x.cvt32());
			shl(x, 3);
			sub(x, t);
			break;
		case 8:
			shl(x, 3);
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
	void modRaw(const ConstDivMod& cdm, uint32_t mode, const Xbyak::Reg32& x)
	{
		if (d_ >= 0x80000000) {
			modName[mode] = "cmp";
			mov(eax, x);
			sub(eax, d_);
			cmovc(eax, x); // x < d ? x : x-d
			return;
		}
		if (cdm.c_ <= 0xffffffff) {
			mov(eax, x);
			if (cdm.c_ > 1) {
				modName[mode] = "mul+shr";
				mov(edx, cdm.c_);
				mul(rdx);
				shr(rax, cdm.a_);

				x_sub_qd(x);
			} else {
				// d is 2-power
				modName[mode] = "and";
				mov(eax, x);
				and_(eax, d_ - 1);
			}
			return;
		}
		switch (mode) {
		case 0:
			modName[0] = "my1";
			mov(eax, cdm.c_ & 0xffffffff);
			imul(rax, x.cvt64());
			shr(rax, 32);
			add(rax, x.cvt64());
			shr(rax, cdm.a_ - 32);

			x_sub_qd(x);
			break;

		case 1:
			modName[1] = "my2";
			mov(edx, x);
			fast_muli(rdx, cdm.c2_ & 0xffffffff, rax);
			shr(rdx, cdm.a2_);
			fast_muli(rdx, d_, rax);
			mov(eax, x);
			sub(rax, rdx);
			lea(edx, ptr[eax + d_]);
			cmovc(eax, edx);
			break;

		case MOD_FUNC_N-1:
		default:
			modName[MOD_FUNC_N-1] = "gcc";
			// generated asm code by gcc/clang
			mov(edx, x);
			mov(eax, cdm.c_ & 0xffffffff);
			imul(rax, rdx);
			shr(rax, 32);
			sub(edx, eax);
			shr(edx, 1);
			add(eax, edx);
			shr(eax, cdm.a_ - 33);

			x_sub_qd(x);
			break;
		}
	}
	bool init(uint32_t d, uint32_t lpN = 1)
	{
		using namespace Xbyak;
		using namespace Xbyak::util;

		d_ = d;
		ConstDivMod cdm;
		if (!cdm.init(d)) return false;
		cdm.put();
		a_ = cdm.a_;
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
				(this->*f)(cdm, bestMode, x);
			}
			for (uint32_t mode = 0; mode < N; mode++) {
				align(32);
				lpTbl[mode] = getCurr<FuncType>();
				StackFrame sf(this, 1, 3|UseRDX);
				const Reg32 n = sf.p[0].cvt32();
				const Reg32 sum = sf.t[0].cvt32();
				const Reg32 x = sf.t[1].cvt32();
				const Reg32 t = sf.t[2].cvt32();
				xor_(sum, sum);
				xor_(x, x);
				align(32);
				Label lpL;
				L(lpL);
				mov(t, x);
				for (uint32_t j = 0; j < lpN; j++) {
					(this->*f)(cdm, mode, t);
					add(sum, eax);
					add(t, sum);
				}
				add(x, 1);
				dec(n);
				jnz(lpL);
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
	void put() const
	{
		printf("Gen d=%u(0x%08x) a=%u divd=%p\n", d_, d_, a_, divd);
	}
};

#elif defined(CONST_DIV_GEN_AARCH64)

static const size_t DIV_FUNC_N = 3;
static const size_t MOD_FUNC_N = 4;

static inline Xbyak_aarch64::WReg cvt32(const Xbyak_aarch64::Reg& x)
{
	return Xbyak_aarch64::WReg(x.getIdx());
}

struct ConstDivModGen : Xbyak_aarch64::CodeGenerator {
	FuncType divd = nullptr;
	FuncType divLp[DIV_FUNC_N] = {};
	const char *divName[DIV_FUNC_N] = {};
	static const int bestDivMode = 0;
	uint32_t d_ = 0;
	uint32_t a_ = 0;

	FuncType modd = nullptr;
	FuncType modLp[MOD_FUNC_N];
	const char *modName[MOD_FUNC_N] = {};
	static const int bestModMode = 0;

	// input x
	// output x = x/d
	// use w9, w10
	void divRaw(const ConstDivMod& cdm, uint32_t mode, const Xbyak_aarch64::XReg& x)
	{
		using namespace Xbyak_aarch64;
		const auto wx = cvt32(x);
		if (d_ >= 0x80000000) {
			divName[mode] = "cmp";
			mov_imm(w9, uint32_t(d_));
			cmp(x, x9);
			cset(x, HS); // Higher or Same
			return;
		}
		if (cdm.c_ > 1) {
			mov_imm(w9, uint32_t(cdm.c_));
		}
		if (cdm.c_ <= 0xffffffff) {
			if (cdm.c_ > 1) {
				divName[mode] = "mul+shr";
				umull(x, wx, w9);
			} else {
				divName[mode] = "shr";
			}
			lsr(x, x, cdm.a_);
			return;
		}
		switch (mode) {
		case 0:
			divName[0] = "mul64";
			movk(x9, 1, 32); // x9 = c = [1:cH:cL]
			mul(x10, x9, x);
			umulh(x9, x9, x); // [x9:x10] = c * x
			extr(x, x9, x10, cdm.a_); // x = [x9:x10] >> a_
			return;
		case 1:
			divName[1] = "my";
			umull(x9, wx, w9); // x9 = [cH:cL] * x
			add(x, x, x9, LSR, 32); // x += x9 >> 32;
			lsr(x, x, cdm.a_ - 32);
			return;
		default:
			divName[2] = "clang";
			// same code generated by clang
			umull(x9, wx, w9);
			lsr(x9, x9, 32);
			sub(w10, wx, w9);
			add(w9, w9, w10, LSR, 1);
			lsr(wx, w9, cdm.a_ - 33);
			return;
		}
	}

	// x *= y, using t
	void fast_muli(const Xbyak_aarch64::XReg& x, uint32_t y, const Xbyak_aarch64::XReg& t)
	{
		if (x.getIdx() == t.getIdx()) {
			fprintf(stderr, "same regs are not allowed\n");
			exit(1);
		}
		const auto wx = cvt32(x);
		const auto wt = cvt32(t);
		switch (y) {
		default:
			mov_imm(wt, y);
			umull(x, wx, wt);
			break;
		}
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
	void modRaw(const ConstDivMod& cdm, uint32_t mode, const Xbyak_aarch64::XReg& x)
	{
		using namespace Xbyak_aarch64;
		const auto wx = cvt32(x);
		if (d_ >= 0x80000000) {
			modName[mode] = "cmp";
			mov_imm(w9, uint32_t(d_));
			cmp(x, x9);
			csel(x10, xzr, x9, LT); // x10 = LT(cmp(x, x9)) ? 0 : x9
			sub(x, x, x10);
			return;
		}
		if (cdm.c_ > 1) {
			mov_imm(w9, uint32_t(cdm.c_));
		}
		if (cdm.c_ <= 0xffffffff) {
			if (cdm.c_ > 1) {
				modName[mode] = "mul+shr";
				umull(x10, wx, w9);
				lsr(x10, x10, cdm.a_);
				x_sub_qd(x, x10, cdm.d_, x9);
			} else {
				modName[mode] = "shr";
				and_imm(wx, wx, uint32_t(d_-1), w9);
			}
			return;
		}
		switch (mode) {
		case 0:
			modName[0] = "mul64";
			movk(x9, 1, 32); // x9 = c = [1:cH:cL]
			mul(x10, x9, x);
			umulh(x9, x9, x); // [x9:x10] = c * x
			extr(x10, x9, x10, cdm.a_); // x = [x9:x10] >> a_
			x_sub_qd(x, x10, cdm.d_, x9);
			return;
		case 1:
			modName[1] = "my";
			umull(x9, wx, w9); // x9 = [cH:cL] * x
			add(x10, x, x9, LSR, 32); // x += x9 >> 32;
			lsr(x10, x10, cdm.a_ - 32);
			x_sub_qd(x, x10, cdm.d_, x9);
			return;
		case 2:
			modName[2] = "new";
			mov_imm(w9, uint32_t(cdm.c2_));
			umull(x10, wx, w9);
			lsr(x10, x10, cdm.a2_);
			mov_imm(w9, uint32_t(cdm.d_));
			umull(x10, w10, w9);
			sub(x, x, x10); // x - q * d
			add(x9, x, x9); // x - q * d + d
			cmp(x, 0);
			csel(x, x, x9, GT); // x = (x >= 0) ? x - q d : x - q d + d
			return;
		default:
			modName[3] = "clang";
			// same code generated by clang
			umull(x9, wx, w9);
			lsr(x9, x9, 32);
			sub(w10, wx, w9);
			add(w9, w9, w10, LSR, 1);
			lsr(w10, w9, cdm.a_ - 33);
			x_sub_qd(x, x10, cdm.d_, x9);
			return;
		}
	}
	bool init(uint32_t d, uint32_t lpN = 1)
	{
		using namespace Xbyak_aarch64;
		ConstDivMod cdm;
		if (!cdm.init(d)) return false;
		cdm.put();
		d_ = cdm.d_;
		a_ = cdm.a_;
		for (int j = 0; j < 2; j++) {
			auto f = j == 0 ? &ConstDivModGen::divRaw : &ConstDivModGen::modRaw;
			const uint32_t N = j == 0 ? DIV_FUNC_N : MOD_FUNC_N;
			int bestMode = j == 0 ? bestDivMode : bestModMode;
			auto funcPtr = j == 0 ? &divd : &modd;
			auto lpTbl = j == 0 ? divLp : modLp;
			{
				align(32);
				*funcPtr = getCurr<FuncType>();
				(this->*f)(cdm, bestMode, x0);
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
					(this->*f)(cdm, mode, t);
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
	void put() const
	{
		printf("Gen d=%u(0x%08x) a=%u divd=%p\n", d_, d_, a_, divd);
	}
};

#endif

} // constdiv

