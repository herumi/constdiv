int test(uint32_t ret) {
  for (int i = 0; i < 1000000000; i++) {
    ret ^= (i ^ ret) / 7;
    ret ^= (i ^ ret) / 19;
    ret ^= (i ^ ret) / 107;
  }
  return ret;
}
