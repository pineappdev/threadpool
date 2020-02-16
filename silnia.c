#include "threadpool.h"
#include "future.h"
#include "utils.h"
#include "err.h"
#include <unistd.h>

typedef struct arg
{
    uint64_t multiplicand;
    uint64_t multiplier;
} arg_t;

void *multiply(void *arg, size_t red, size_t *size)
{
    if (red == 0 && *size == 0)
    {
        red = 0;
    } // for no warnings

    arg_t *res = (arg_t *)arg;
    res->multiplicand *= res->multiplier;
    ++(res->multiplier);

    *size = sizeof(arg_t *);

    return res;
}

int main()
{
    int n;
    scanf("%d", &n);
    if(n == 0) {
        n = 1;
    }

    callable_t mult;

    thread_pool_t pool;
    thread_pool_init(&pool, 3);

    arg_t arg;
    arg.multiplicand = 1;
    arg.multiplier = 1;

    mult.function = multiply;
    mult.arg = &arg;
    mult.argsz = sizeof(arg_t *);

    future_t *futures = malloc(sizeof(future_t) * n);
    if(futures == NULL) {
        syserr("malloc");
    }

    async(&pool, futures, mult);

    for(int i = 1; i < n; i++) {
        map(&pool, futures + i, futures + i - 1, multiply);
    }

    uint64_t *outcome = await(futures + n - 1);
    printf("%ld\n", *outcome);

    // destroy pool
    free(futures);
    thread_pool_destroy(&pool);

    return 0;
}
