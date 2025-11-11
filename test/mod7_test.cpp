#define CYBOZU_BENCH_CHRONO
#include <cybozu/benchmark.hpp>
#include <stdint.h>

//Mod d=7(0x00000007) a=32 c32=0x24924925 over=0 cmp=0 e=00000003
//#define D 7
#define D 12345
//#define D 1234609
//#define D 1073741823

#if D == 7
const uint32_t d_ = D;
const uint32_t a_ = 32;
const uint32_t s_ = 1;
const uint64_t c_ = 0x24924925;
const bool cmp_ = false;
#endif
#if D == 12345
const uint32_t d_ = D;
const uint32_t a_ = 29;
const uint32_t s_ = 0;
const uint64_t c_ = 0xa9e1;
const bool cmp_ = false;
#endif
#if D == 1234609
//Mod d=1234609(0x0012d6b1) a=32 c32=0x00000d97 over=0 cmp=0 e=00039f67
const uint32_t d_ = D;
const uint32_t a_ = 32;
const uint32_t s_ = 1;
const uint64_t c_ = 0x00000d97;
const bool cmp_ = false;
#endif
#if D == 1073741823
const uint32_t d_ = D;
const uint32_t a_ = 59;
const uint32_t s_ = 29;
const uint64_t c_ = 536870913;
const bool cmp_ = false;
#endif

#ifdef _WIN32
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

extern "C" {
uint32_t moddorg(uint32_t x);
uint32_t moddnew(uint32_t x);
}

NOINLINE uint32_t moddorgC(uint32_t x)
{
	return x % d_;
}

NOINLINE uint32_t moddnewC(uint32_t x)
{
	uint64_t v = (x * c_) >> a_;
	v *= d_;
	if (x >= v) {
		return x - v;
	} else {
		return x - v + d_;
	}
}
/*
	# optimized mod7new
    mov eax, edi
    mov ecx, edi
    imul    rax, rax, 613566757
    shr rax, 32

    lea edx, 0[0+rax*8]
    sub edx, eax # edx = v = (x//d) * d
    sub edi, edx # x - v
    mov edx, 7
    sbb ecx, ecx
    and edx, ecx
    lea eax, [edi+edx]

#   lea edx, 0[0+rax*8]
#   sub edx, eax
#   lea eax, 7[rdi]
#   sub ecx, edx
#   sub eax, edx
#   cmp edi, edx
#   cmovnb  eax, ecx
    ret
*/

typedef uint32_t (*DivFunc)(uint32_t);

uint32_t g_N = uint32_t(1e8);
const int C = 10;

uint32_t loop1(DivFunc f)
{
	uint32_t sum = 0;
	for (uint32_t x = 0; x < g_N; x++) {
		sum += f(x);
//		sum += f(x * 2);
//		sum += f(x * 3);
	}
	return sum;
}

int main()
{
	uint32_t r0 = 0, r1 = 0;
	CYBOZU_BENCH_C("org ", C, r0 += loop1, moddorg);
	CYBOZU_BENCH_C("new ", C, r1 += loop1, moddnew);
//	CYBOZU_BENCH_C("org ", C, r0 += loop1, moddorg);
//	CYBOZU_BENCH_C("new ", C, r1 += loop1, moddnew);
	if (r0 != r1) {
		printf("ERR org2 =0x%08x\n", r1);
	}
}
