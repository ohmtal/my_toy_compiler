#include <cstdio>
#include <iostream>

extern "C" void printi(long long val) { printf("%lld\n", val); }
extern "C" void native_echo(long long value) { std::cout << value << std::endl; }
