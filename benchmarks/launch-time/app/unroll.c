__attribute__((export_name(EXPORT_NAME))) void FUNC_NAME(void) {
    #pragma clang loop unroll_count(10000)
    for (unsigned long long i = 0; i < 10000; i++)
    {
        printf("\n");
    }
    #pragma clang loop unroll_count(10000)
    for (unsigned long long i = 0; i < 10000; i++)
    {
        printf("\n");
    }
    #pragma clang loop unroll_count(10000)
    for (unsigned long long i = 0; i < 10000; i++)
    {
        printf("\n");
    }
    #pragma clang loop unroll_count(10000)
    for (unsigned long long i = 0; i < 10000; i++)
    {
        printf("\n");
    }
    #pragma clang loop unroll_count(3000)
    for (unsigned long long i = 0; i < 3000; i++)
    {
        printf("\n");
    }
}