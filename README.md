# threadpool
Implementation of pool of threads in C

Realisation of threadpool and async/await mechanisms in C. 

Calling 'thread_pool_init' initiates argument pointed to by 'pool' as a new pool, in which there'll be 'pool_size' of threads executing submitted tasks. The credit for pool memory management is taken by the library's user. Correctness of the library's functionality is guaranteed if and only if every pool created by 'thread_pool_init' is destroyed by calling 'thread_pool_destroy' with argument representing that pool.

Calling 'defer(pool, runnable)' orders the pool to execute a task defined by the 'runnable' argument (see threadpool.h for runnable's details).

Function 'function' from 'runnable' is computed by a thread from the 'pool'; calling 'defer' may block the thread executing that 'defer', but only for the need of submitting the task - return from 'defer' is independent from returning from 'function' execution by the pool. In other words, submitting a task should not be connected with its execution - we order a pool to execute a task and return from the submission function regardless of whether the task has already been executed or not.

Using 'defer' and 'threadpool' mechanisms I implemented 'future' (similar to javascript's or C++ mechanisms). 'Future' allows to submit a task (by 'async') and wait until it's done ('await'); using 'map' function, we can wait for the future's outcome and order to compute something using it in yet another 'future'.

'Macierz' (matrix) and 'silnia' (factorial) programs faciliatate above mechanisms, and the corresponding .sh scripts are responsible for carrying out some tests whether those programs work.
