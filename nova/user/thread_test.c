#include <stdio.h>

static inline unsigned syscall1(unsigned w0)
{
    asm volatile(
        "   mov %%esp, %%ecx    ;"
        "   mov $1f, %%edx      ;"
        "   sysenter            ;"
        "1:                     ;"
        : "+a"(w0)
        :
        : "ecx", "edx", "memory");
    return w0;
}

static inline unsigned syscall3(unsigned w0, unsigned w1, unsigned w2)
{
    asm volatile(
        "   mov %%esp, %%ecx    ;"
        "   mov $1f, %%edx      ;"
        "   sysenter            ;"
        "1:                     ;"
        : "+a"(w0)
        : "S"(w1), "D"(w2)
        : "ecx", "edx", "memory");
    return w0;
}

int create_thread(void (*start_routine)(void), void *stack)
{
    return syscall3(4, (unsigned long)start_routine, (unsigned long)stack);
}

void thread_yield(void)
{
    syscall1(5);
}

char stack[0x1000] __attribute__((aligned(16)));

void thread1()
{
    while (1)
    {
        printf("%s running\n", __func__);
        thread_yield();
    }
}

int main()
{
    printf("Main thread running\n");

    create_thread(thread1, stack + sizeof(stack));
    while (1)
    {
        thread_yield();
        printf("I'm back in the main thread\n");
    }
}