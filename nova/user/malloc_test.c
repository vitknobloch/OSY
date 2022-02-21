#include "mem_alloc.h"
#include <stdio.h>

int main()
{
    void *addr1, *addr2, *addr3, *addr4, *addr5;

    addr1 = my_malloc(1032);
    addr2 = my_malloc(512);
    my_free(addr1);
    addr3 = my_malloc(512);
    addr4 = my_malloc(512);
    addr5 = my_malloc(512);
    my_free(addr2);
    my_free(addr3);
    my_free(addr4);
    my_free(addr5);
    printf("\nProgram done\n");
    return 0;
}
