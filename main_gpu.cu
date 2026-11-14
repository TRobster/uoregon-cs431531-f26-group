#include <cstdio>

int main()
{
    int n = 0;
    cudaGetDeviceCount(&n);
    printf("cuda devices %d\n", n);
}
