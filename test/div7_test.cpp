#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
//#define CYBOZU_BENCH_CHRONO
#include <cybozu/benchmark.hpp>
#include <cybozu/option.hpp>
#define CYBOZU_TEST_DISABLE_AUTO_RUN
#include <cybozu/test.hpp>
#include "constdiv.hpp"
#include <math.h>
#include <mutex>

#include <atomic>
//#define COUNT_33BIT

extern "C" {

uint32_t div7org(uint32_t x);
uint32_t div7org2(uint32_t x);

uint32_t mod7org(uint32_t x);
uint32_t mod7org2(uint32_t x);


} // extern "C"

using namespace constdiv;

uint32_t g_N = uint32_t(1e8);
const int C = 10;
uint32_t LP_N = 3;

uint32_t loop1(FuncType f)
{
	uint32_t sum = 0;
	for (uint32_t x = 0; x < g_N; x++) {
		uint32_t t = x;
		for (uint32_t i = 0; i < LP_N; i++) {
			sum += f(t);
			t += sum;
		}
	}
	return sum;
}

FuncType gen = 0;

uint32_t g_d;

uint32_t divdorg(uint32_t x)
{
	return x / g_d;
}

uint32_t modorg(uint32_t x)
{
	return x % g_d;
}


uint32_t loopDivOrg(uint32_t d)
{
	puts("loopDivOrg");
	uint32_t r0 = 0, r1 = 0;
	const int D = 10;
	if (d == 7) {
		CYBOZU_BENCH_C("org ", C/D, r0 += loop1, div7org);
		CYBOZU_BENCH_C("org2", C/D, r1 += loop1, div7org2);
		if (r0 != r1) {
			printf("ERR org2 =0x%08x\n", r1);
		}
	} else {
		CYBOZU_BENCH_C("org ", C/D, r0 += loop1, divdorg);
	}
	r0 *= D;
	printf("org  =0x%08x\n", r0);
	return r0;
}

uint32_t loopModOrg(uint32_t d)
{
	puts("loopModOrg");
	uint32_t r0 = 0, r1 = 0;
	const int D = 10;
	if (d == 7) {
		CYBOZU_BENCH_C("org ", C/D, r0 += loop1, mod7org);
		CYBOZU_BENCH_C("org2", C/D, r1 += loop1, mod7org2);
		if (r0 != r1) {
			printf("ERR org2 =0x%08x\n", r1);
		}
	} else {
		CYBOZU_BENCH_C("org ", C/D, r0 += loop1, modorg);
	}
	r0 *= D;
	printf("org  =0x%08x\n", r0);
	return r0;
}

#ifdef CONST_DIV_GEN
void loopDiv(const ConstDivModGen& cdmg, uint32_t r0)
{
	puts("loopDiv");
	uint32_t rs[DIV_FUNC_N] = {};
	for (size_t i = 0; i < DIV_FUNC_N; i++) {
		FuncType f = cdmg.divLp[i];
		char buf[64];
		snprintf(buf, sizeof(buf), "%10s", cdmg.divName[i]);
		CYBOZU_BENCH_C(buf, C, rs[i] += f, g_N);
	}
	for (size_t i = 0; i < DIV_FUNC_N; i++) {
		printf("rs[%zd]=0x%08x ok=0x%08x %s\n", i, rs[i], r0, rs[i] == r0 ? "ok" : "ng");
	}
}

void loopMod(const ConstDivModGen& cdmg, uint32_t r0)
{
	puts("loopMod");
	uint32_t rs[MOD_FUNC_N] = {};
	for (size_t i = 0; i < MOD_FUNC_N; i++) {
		FuncType f = cdmg.modLp[i];
		char buf[64];
		snprintf(buf, sizeof(buf), "%10s", cdmg.modName[i]);
		CYBOZU_BENCH_C(buf, C, rs[i] += f, g_N);
	}
	for (size_t i = 0; i < MOD_FUNC_N; i++) {
		printf("rs[%zd]=0x%08x ok=0x%08x %s\n", i, rs[i], r0, rs[i] == r0 ? "ok" : "ng");
	}
}

template<class DM>
void checkone(const DM& dm, uint32_t x)
{
	uint32_t d = dm.d_;
	uint32_t q = x / d;
	uint32_t r = x % d;
	uint32_t a = dm.divd(x);
	uint32_t b = dm.modd(x);
	if (q != a) {
		printf("ERR q x=%u expected %u bad %u\n", x, q, a);
		exit(1);
	}
	if (r != b) {
		printf("ERR r x=%u expected %u bad %u\n", x, r, b);
		exit(1);
	}
}

template<class DM>
void checkd(uint32_t d, bool allx) {
	DM dm;
	if (!dm.init(d)) {
		printf("ERR dm.init d=%u\n", d);
		exit(1);
	}
	dm.put();
	if (allx) {
#pragma omp parallel for
		for (int64_t x_ = 0; x_ <= 0xffffffff; x_++) {
			uint32_t x = uint32_t(x_);
			checkone(dm, x);
		}
		return;
	}
	const uint32_t xTbl[] = {
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 16, 17, 19,
		32, 64, 128, 256, 512, 641, 10287,
		65535, 65536, 65537,
		(1u<<20) - 1, (1u<<20), (1u<<20) + 1,
		(1u<<30) - 1, (1u<<30), (1u<<30) + 1,
		(1u<<31) - 1, (1u<<31), (1u<<31) + 1,
		uint32_t(-3), uint32_t(-2), uint32_t(-1),
	};
	dm.put();
	for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(xTbl); i++) {
		uint32_t x = xTbl[i];
		checkone(dm, x);
	}
}
#endif

// use d if !alld
template<class DM>
void checkall(uint32_t d, bool alld, bool allx)
{
	if (alld) {
		printf("checkall alld\n");
#pragma omp parallel for
		for (int d = 1; d <= 0x7fffffff; d++) {
			checkd<DM>(d, allx);
		}
		return;
	}
	printf("checkall d=%u\n", d);
	checkd<DM>(d, allx);
}

CYBOZU_TEST_AUTO(log)
{
	for (uint32_t x = 1; x < 300; x++) {
		uint32_t y = int(ceil(log2(double(x))));
		CYBOZU_TEST_EQUAL(ceil_ilog2(x), y);
	}
}

void findSmallC()
{
	const size_t n = 16;
	uint32_t dTbl1[n] = {}, dTbl2[n] = {};
	uint32_t find1 = 0, find2 = 0;
	std::mutex mut;
#pragma omp parallel for
	for (int d = 1; d <= 0x7fffffff; d++) {
		ConstDivMod cdm;
		cdm.init(d);
		uint64_t c = cdm.c_;
		std::lock_guard<std::mutex> g(mut);
		if (c < n && ((find1 & (1<<c)) == 0)) {
			find1 |= 1<<c;
			dTbl1[c] = d;
		}
		c = cdm.c2_;
		if (c < n && ((find2 & (1<<c)) == 0)) {
			find2 |= 1<<c;
			dTbl2[c] = d;
		}
	}
	puts("d table");
	for (size_t i = 0; i < n; i++) {
		printf("d1[%zd]=%u\n", i, dTbl1[i]);
	}
	for (size_t i = 0; i < n; i++) {
		printf("d2[%zd]=%u\n", i, dTbl2[i]);
	}
}

struct MinPair {
	uint32_t a;
	int d;
};

struct MaxPair {
	uint32_t a;
	int d;
};

#pragma omp declare reduction( \
	minpair : MinPair : \
	omp_out = (omp_in.a < omp_out.a) ? omp_in : omp_out \
) initializer(omp_priv = {64, 0})

#pragma omp declare reduction( \
	maxpair : MaxPair : \
	omp_out = (omp_in.a > omp_out.a) ? omp_in : omp_out \
) initializer(omp_priv = {0, 0})


/*
min a = 57 at d = 19173962
max a = 63 at d = 1533916890
*/
void exec_range_a()
{
	MinPair min_p = { 64, 0 };
	MaxPair max_p = { 0, 0 };

#if 1
	for (int d = 1; d <= 0x7fffffff; d++) {
		ConstDivMod cdm;
		cdm.init(d);
		if (cdm.over_) {
			if (cdm.a_ < min_p.a) {
				min_p.a = cdm.a_;
				min_p.d = d;
			}
			if (cdm.a_ > max_p.a) {
				max_p.a = cdm.a_;
				max_p.d = d;
			}
		}
	}
#else
#pragma omp parallel for reduction(minpair:min_p) reduction(maxpair:max_p)
	for (int d = 1; d <= 0x7fffffff; d++) {
		ConstDivMod cdm;
		cdm.init(d);
		if (cdm.over_) {
			MinPair curMin = {cdm.a_, d};
			min_p = curMin;

			MaxPair curMax = {cdm.a_, d};
			max_p = curMax;
		}
	}
#endif
	printf("min d=%d a=%u\n", min_p.d, min_p.a);
	printf("max d=%d a=%u\n", max_p.d, max_p.a);
}

int main(int argc, char *argv[])
	try
{
	cybozu::Option opt;
	uint32_t d;
	bool alld;
	bool unitTest;
	bool benchOnly;
	bool find33bit;
	bool count33bit;
	bool allx;
	bool testc;
	bool findc;
	bool range_a;
	opt.appendOpt(&d, 7, "d", "divisor");
	opt.appendOpt(&LP_N, 3, "lp", "loop counter");
	opt.appendOpt(&g_N, uint32_t(1e8), "N", "N");
#ifdef CONST_DIV_GEN
	int divMode;
	opt.appendOpt(&divMode, ConstDivModGen::bestDivMode, "divmode", "div mode");
#endif
	opt.appendBoolOpt(&alld, "alld", "check all d");
	opt.appendBoolOpt(&unitTest, "ut", "unit test only");
	opt.appendBoolOpt(&benchOnly, "bench", "benchmark only");
	opt.appendBoolOpt(&find33bit, "f33", "find 33bit c");
	opt.appendBoolOpt(&count33bit, "c33", "count 33bit c");
	opt.appendBoolOpt(&allx, "allx", "test all x");
	opt.appendBoolOpt(&testc, "ctest", "test C");
	opt.appendBoolOpt(&findc, "findc", "find small c");
	opt.appendBoolOpt(&range_a, "range_a", "range of a");
	opt.appendHelp("h");
	if (opt.parse(argc, argv)) {
		opt.put();
	} else {
		opt.usage();
	}
	if (unitTest) {
		return cybozu::test::autoRun.run(argc, argv);
	}
	g_d = d;
	if (find33bit) {
		for (int d = g_d; d <= 0x7fffffff; d += 2) {
			ConstDivMod cdm;
			cdm.init(d);
			if (cdm.c_ > 0xffffffff) {
				printf("find\n");
				cdm.put();
				return 1;
			}
		}
		return 0;
	}
	if (findc) {
		findSmallC();
		return 0;
	}
	if (range_a) {
		exec_range_a();
		return 0;
	}
	if (count33bit) {
		std::atomic<uint32_t> count{0};
		puts("check alld");
#pragma omp parallel for
		for (int d = 1; d <= 0x7fffffff; d++) {
			ConstDivMod cdm;
			if (!cdm.init(d)) {
				printf("err d=%d\n", d); exit(1);
			}
			if (cdm.c_ > 0xffffffff) {
				count++;
			}
		}
		printf("count=%u (%.2f)\n", count.load(), count.load() / double(0x7fffffff - 1));
		return 0;
	}
	if (testc) {
		puts("checkall ConstDivMod");
		checkall<ConstDivMod>(d, alld, allx);
		return 0;
	} else {
		puts("checkall ConstDivModGen");
		checkall<ConstDivModGen>(d, alld, allx);
	}

#ifdef CONST_DIV_GEN
	ConstDivModGen cdmg;
	if (!cdmg.init(d, LP_N)) {
		printf("err cdmg d=%u\n", d);
		return 1;
	}
#endif
	uint32_t divOk = 0;
	uint32_t modOk = 0;
	if (!benchOnly) {
		divOk = loopDivOrg(d);
		modOk = loopModOrg(d);
	}
#ifdef CONST_DIV_GEN
	if (benchOnly) {
		CYBOZU_BENCH_C("bench", C, cdmg.divLp[divMode], g_N);
		return 0;
	}
	cdmg.put();
	cdmg.dump();
	puts("benchmark");
	loopDiv(cdmg, divOk);
	loopMod(cdmg, modOk);

#endif
} catch (std::exception& e) {
	printf("err e=%s\n", e.what());
	return 1;
}
