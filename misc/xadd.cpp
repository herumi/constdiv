#include <stdio.h>
#include <stdint.h>
#include <math.h>

const uint32_t M = 0xffffffff;
const uint32_t d = 0x70000001;
/*
y = x/d
z = (x+b)/S
z = y or z = y+1
d = 0x7fffffff, b in [2, S+1]
d = 0x7ffffffe, b in [4, S+2]
d = 0x7ffffffd, b in [6, S+3]
d = 0x7ffffff0, b in [32, S+16]
d = 0x7fffff00, b in [32*16, S+32*8]
*/

constexpr uint32_t ceil_log2(uint32_t x)
{
	return uint32_t(ceil(log2(x)));
}

uint32_t modd(uint32_t x)
{
	const uint32_t s = ceil_log2(d);
	const uint32_t S = 1u << s;
	const uint32_t b = 0x16000000;//0x50000000;//S+32*8-1;
	uint32_t y = x / d;
	uint32_t z = (uint64_t(x)+b) / S;
	// assert(z == y or z == y+1)
	if (!(z == y || z == y+1)) {
		printf("x=0x%08x y=0x%08x z=0x%08x diff=%d\n", x, y, z, z - y);
		exit(1);
	}
	return 0;
}

int main()
{
	#pragma omp parallel for
	for (int64_t i = 0; i <= M; i++) {
		uint32_t x = uint32_t(i);
		modd(x);
	}
	puts("OK");
}


