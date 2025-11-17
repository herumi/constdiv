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

#include <atomic>
//#define COUNT_33BIT
//#define COUNT_DIFF

extern "C" {

uint32_t div7org(uint32_t x);
uint32_t div7org2(uint32_t x);

} // extern "C"

using namespace constdiv;

uint32_t g_N = uint32_t(1e8);
const int C = 10;
uint32_t LP_N = 3;

uint32_t loop1(DivFunc f)
{
	uint32_t sum = 0;
	for (uint32_t x = 0; x < g_N; x++) {
		for (uint32_t i = 0; i < LP_N; i++) {
			sum += f(x);
		}
	}
	return sum;
}

DivFunc gen = 0;

uint32_t g_d;

uint32_t divdorg(uint32_t x)
{
	return x / g_d;
}

uint32_t loopOrg(uint32_t d)
{
	uint32_t r0 = 0, r1 = 0;
	if (d == 7) {
		CYBOZU_BENCH_C("org ", C, r0 += loop1, div7org);
		CYBOZU_BENCH_C("org2", C, r1 += loop1, div7org2);
		if (r0 != r1) {
			printf("ERR org2 =0x%08x\n", r1);
		}
	} else {
		CYBOZU_BENCH_C("org ", C, r0 += loop1, divdorg);
	}
	printf("org  =0x%08x\n", r0);
	return r0;
}

#ifdef CONST_DIV_GEN
void loopGen(const ConstDivGen& cdg, uint32_t r0)
{
	uint32_t rs[FUNC_N] = {};
	for (size_t i = 0; i < FUNC_N; i++) {
		DivFunc f = cdg.divLp[i];
		char buf[64];
		snprintf(buf, sizeof(buf), "%10s", cdg.name[i]);
		CYBOZU_BENCH_C(buf, C, rs[i] += f, g_N);
	}
	for (size_t i = 0; i < FUNC_N; i++) {
		printf("rs[%zd]=0x%08x %s\n", i, rs[i], rs[i] == r0 ? "ok" : "ng");
	}
}

void checkd(uint32_t d) {
	printf("test x/%u for all x\n", d);
	ConstDivGen cdg;
	cdg.init(d);
#pragma omp parallel for
	for (int64_t x_ = 0; x_ <= 0xffffffff; x_++) {
		uint32_t x = uint32_t(x_);
		uint32_t o = x / d;
		uint32_t a =cdg.divd(x);
		if (o != a) {
			printf("ERR x=%u o=%u a=%u\n", x, o, a);
			exit(1);
		}
	}
	puts("ok");
}
#endif

void checkmod(ConstMod& cm) {
	printf("test x %% %u for all x\n", cm.d_);
#pragma omp parallel for
	for (int64_t x_ = 0; x_ <= 0xffffffff; x_++) {
		uint32_t x = uint32_t(x_);
		uint32_t o = x % cm.d_;
		uint32_t a = cm.modd(x);
		if (o != a) {
			cm.put();
			printf("ERR x=%u o=%u a=%u\n", x, o, a);
//			exit(1);
		}
	}
	puts("ok");
}

CYBOZU_TEST_AUTO(log)
{
	for (uint32_t x = 1; x < 300; x++) {
		uint32_t y = int(ceil(log2(double(x))));
		CYBOZU_TEST_EQUAL(ceil_ilog2(x), y);
	}
}

int main(int argc, char *argv[])
	try
{
	cybozu::Option opt;
	uint32_t d;
	bool alld;
	bool unitTest;
	bool benchOnly;
	bool mod;
	bool find33bit;
	opt.appendOpt(&d, 7, "d", "divisor");
	opt.appendOpt(&LP_N, 3, "lp", "loop counter");
	opt.appendOpt(&g_N, uint32_t(1e8), "N", "N");
#ifdef CONST_DIV_GEN
	int mode;
	opt.appendOpt(&mode, ConstDivGen::bestMode, "mode", "mode");
#endif
	opt.appendBoolOpt(&alld, "alld", "check all d");
	opt.appendBoolOpt(&unitTest, "ut", "unit test only");
	opt.appendBoolOpt(&benchOnly, "bench", "benchmark only");
	opt.appendBoolOpt(&mod, "mod", "mod");
	opt.appendBoolOpt(&find33bit, "f33", "find 33bit c");
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
			ConstDiv cd;
			cd.init(d);
			if (cd.c_ > 0xffffffff) {
				printf("find\n");
				cd.put();
				return 1;
			}
		}
		return 0;
	}
	if (mod) {
		if (alld) {
			puts("mod check alld");
#pragma omp parallel for
			for (int d = 1; d <= 0x7fffffff; d++) {
				ConstMod cm;
				if (!cm.init(d)) {
					printf("INIT ERR %x\n", d);
					continue;
				}
				if (cm.c_ > 0xffffffff) {
					printf("d=%d cm.c_ over\n", d);
				}
				checkmod(cm);
			}
		} else {
			puts("mod check d");
			ConstMod cm;
			if (!cm.init(d)) {
				printf("err d=%d\n", d); exit(1);
			}
			cm.put();
			if (cm.c_ > 0xffffffff) {
				printf("d=%d cm.c_ over\n", d);
			}
			checkmod(cm);
		}
		return 0;
	}
	if (alld) {
#if defined(COUNT_33BIT) || defined(COUNT_DIFF)
		std::atomic<uint32_t> count{0};
#endif
		puts("check alld");
#pragma omp parallel for
		for (int d = 1; d <= 0x7fffffff; d++) {
			ConstDiv cd;
			if (!cd.init(d)) {
				printf("err d=%d\n", d); exit(1);
			}
#ifdef COUNT_33BIT
			if (cd.c_ > 0xffffffff) {
				count++;
			}
#endif
#ifdef COUNT_DIFF
			// a = ceil(log2(d)) + 32 is a sufficient condition but not optimal.
			// e.g., d = 18, 23, ...
			uint32_t b = ConstDiv::ceil_ilog2(d) + 32;
			if (b != cd.a_) {
				printf("d=%u a=%u b=%u %d %d\n", d, cd.a_, b, cd.c_ > 0xffffffff, ((uint64_t(1) << b) + d - 1)/d > 0xffffffff);
				count++;
			}
#endif
		}
#if defined(COUNT_33BIT) || defined(COUNT_DIFF)
		printf("count=%u (%.2f)\n", count.load(), count.load() / double(0x7fffffff - 1));
#endif
		puts("ok");
#ifdef CONST_DIV_GEN
		const uint32_t tbl[] = {
			1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 16, 17, 19,
			32, 64, 128, 256, 512, 641, 10287,
			65535, 65536, 65537,
			(1u<<20) - 1, (1u<<20), (1u<<20) + 1,
			(1u<<30) - 1, (1u<<30), (1u<<30) + 1,
			(1u<<31) - 1, (1u<<31), (1u<<31) + 1,
			uint32_t(-3), uint32_t(-2), uint32_t(-1),
		};
		for (size_t i = 0; i < CYBOZU_NUM_OF_ARRAY(tbl); i++) {
			checkd(tbl[i]);
		}
#endif
		return 0;
	}

#ifdef CONST_DIV_GEN
	ConstDivGen cdg;
	if (!cdg.init(d, LP_N)) {
		printf("err cdg d=%u\n", d);
		return 1;
	}
#endif
	uint32_t r0 = 0;
	if (!benchOnly) {
		r0 = loopOrg(d);
	}
#ifdef CONST_DIV_GEN
	if (benchOnly) {
		CYBOZU_BENCH_C("bench", C, cdg.divLp[mode], g_N);
		return 0;
	}
	cdg.put();
	cdg.dump();
	loopGen(cdg, r0);

	checkd(d);
#endif
} catch (std::exception& e) {
	printf("err e=%s\n", e.what());
	return 1;
}
