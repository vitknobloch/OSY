#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define HEXA_DIGIT_MASK 0xf

int _read(int fileno, char *dest, int size);
int _write(int fileno, void *data, int size);

int isnum(char ch)
{
    return ch >= '0' && ch <= '9';
}

int isspc(char ch)
{
    return ch == ' ' || ch == '\n';
}

int _read(int fileno, char *dest, int size)
{
    int ret;

    asm volatile(
        "mov $3, %%eax;"
        "mov %1, %%ebx;"
        "mov %2, %%ecx;"
        "mov %3, %%edx;"
        "int $0x80;"
        "mov %%eax, %0;"
        : "=r"(ret)
        : "r"(fileno), "r"(dest), "r"(size)
        : "cc", "memory", "ax", "bx", "cx", "dx");

    return ret;
}

int _write(int fileno, void *data, int size)
{
    int ret;

    asm volatile(
        "mov $4, %%eax;"
        "mov %1, %%ebx;"
        "mov %2, %%ecx;"
        "mov %3, %%edx;"
        "int $0x80;"
        "mov %%eax, %0;"
        : "=r"(ret)
        : "r"(fileno), "r"(data), "r"(size)
        : "cc", "memory", "ax", "bx", "cx", "dx");

    return ret;
}

void my_exit(int exit_status)
{
    asm volatile(
        "mov $1, %%eax;"
        "mov %0, %%ebx;"
        "int $0x80;"
        :
        : "r"(exit_status)
        : "cc", "ax", "bx");
}

static void print(unsigned num)
{
    char buf[20] = {'0', 'x'};
    int digit_index = 2;
    for (int i = 28; i >= 0; i -= 4)
    {
        unsigned hexa_digit = (num >> i) & HEXA_DIGIT_MASK;
        if (hexa_digit == 0)
        {
            if (digit_index > 2)
                buf[digit_index++] = '0';
        }
        else if (hexa_digit < 10)
            buf[digit_index++] = '0' + hexa_digit;
        else
            buf[digit_index++] = 'a' + hexa_digit - 10;
    }
    if (digit_index == 2)
        buf[digit_index++] = '0';
    buf[digit_index++] = '\n';

    int ret = _write(STDOUT_FILENO, buf, digit_index);
    if (ret == -1)
        my_exit(1); // TODO your new exit
}

int main()
{
    char buf[20];
    unsigned num = 0;
    int i;
    int num_digits = 0;
    unsigned chars_in_buffer = 0;

    for (/* no init */; /* no end condition */; i++, chars_in_buffer--)
    {
        if (chars_in_buffer == 0)
        {
            int ret = _read(STDIN_FILENO, buf, sizeof(buf));
            if (ret < 0)
                my_exit(1);
            i = 0;
            chars_in_buffer = ret;
        }
        if (
            num_digits > 0 && (chars_in_buffer == 0 /* EOF */ || !isnum(buf[i])))
        {
            print(num);
            num_digits = 0;
            num = 0;
        }
        if (
            chars_in_buffer == 0 /* EOF */
            || (!isspc(buf[i]) && !isnum(buf[i])))
            my_exit(0);

        if (isnum(buf[i]))
        {
            num = num * 10 + buf[i] - '0';
            num_digits++;
        }
    }
}

void _start(void)
{
    int ret = main();
    my_exit(ret);
}