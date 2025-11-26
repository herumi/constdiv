#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

void inspect(uint32_t d, uint32_t a, uint32_t s)
{
	int64_t A = int64_t(1) << a;
	int64_t c = (A + d - 1) / d;
	int64_t e = int64_t(d * c) - A;
	uint32_t S = 1 << s;
	printf("d=%u a=%u s=%u c=%" PRIi64 " e=%" PRIi64 "\n", d, a, s, c, e);
	int64_t min = INT64_MAX;
	int64_t max = INT64_MIN;
	uint32_t xmin = 0;
	uint32_t xmax = 0;
	const int64_t M = 0xffffffff;
//	const int64_t M = 1610;
	for (int64_t _x = 0; _x <= M; _x++) {
		uint32_t x = uint32_t(_x);
		int32_t q = x / d;
		int32_t r = x % d;
//		int32_t H = x / S;
		int32_t L = x % S;
		int64_t v = q * e + (r-L) * c;
//		printf("%u %" PRIi64 "\n", x, v);
		if (v < min) {
			min = v;
			xmin = x;
//			printf("x=%u min=%" PRIi64 "\n", uint32_t(x), min);
		}
		if (v > max) {
			max = v;
			xmax = x;
		}
	}
	printf("max=%" PRIi64 " (x=%u)\n", max, xmax);
	printf("min=%" PRIi64 " (x=%u)\n", min, xmin);
	printf("candi1=%" PRIi64 "\n", (M/d)*d/S * S);
	printf("candi2=%" PRIi64 "\n", (M/S)*S/d*d);
}

int main()
{
#if 1
	inspect(123, 31, 5);
	inspect(12345, 29, 12);
	inspect(0xffff, 31, 2);
#endif
	inspect(0xfffffff, 55, 27);
//	inspect((1<<30)-1,59,29);
}
