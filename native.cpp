#include <cstdio>
#include <iostream>

extern "C"
void printi(long long val)
{
    printf("%lld\n", val);
}

extern "C" void echo(long long value) {
    std::cout << value << std::endl;
}
