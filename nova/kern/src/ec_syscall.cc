#include "ec.h"
#include "ptab.h"
#include "string.h"

typedef enum
{
    sys_print = 1,
    sys_sum = 2,
    sys_break = 3,
    sys_thr_create = 4,
    sys_thr_yield = 5,
} Syscall_numbers;

bool allocate_pages(mword old_page, mword new_page);
void free_pages(mword old_page, mword new_page);
void zero_area(mword old_break, mword old_page, mword size);

void Ec::syscall_handler(uint8 a)
{
    // Get access to registers stored during entering the system - see
    // entry_sysenter in entry.S
    Sys_regs *r = current->sys_regs();
    Syscall_numbers number = static_cast<Syscall_numbers>(a);

    switch (number)
    {
    case sys_print:
    {
        char *data = reinterpret_cast<char *>(r->esi);
        unsigned len = r->edi;
        for (unsigned i = 0; i < len; i++)
            printf("%c", data[i]);
        break;
    }
    case sys_sum:
    {
        // Naprosto nepotřebné systémové volání na sečtení dvou čísel
        int first_number = r->esi;
        int second_number = r->edi;
        r->eax = first_number + second_number;
        break;
    }
    case sys_break:
    {
        // printf("Current break: %lx\n", current->break_current);
        // printf("New break: %lx\n", r->esi);
        mword old_break = current->break_current;
        mword new_break = r->esi;
        if (new_break == 0)
        {
            r->eax = old_break;
            break;
        }
        if (new_break < current->break_min || new_break > 0xBFFFF000)
        {
            r->eax = 0;
            break;
        }
        mword old_page = (old_break - 1) >> 12;
        mword new_page = (new_break - 1) >> 12;
        if (new_page > old_page)
        {
            zero_area(old_break, old_page, ((old_page + 1) << 12) - old_break);
            if (!allocate_pages(old_page, new_page))
            {
                r->eax = 0;
                break;
            }
        }
        else if (new_page < old_page)
        {
            free_pages(old_page, new_page);
        }
        else if (new_break > old_break)
        {
            zero_area(old_break, old_page, new_break - old_break);
        }

        current->break_current = new_break;
        r->eax = old_break;
        break;
    }
    case sys_thr_create:
    {
        mword start_routine = r->esi;
        mword stack_top = r->edi;
        Ec *new_thread = new Ec(start_routine, stack_top);
        new_thread->regs.edx = start_routine;
        new_thread->next = Ec::current->next;
        Ec::current->next = new_thread;
        r->eax = 0;
        break;
    }
    case sys_thr_yield:
    {
        Ec *last = current;
        while (last->next && last->next != current)
            last = last->next;
        last->next = current;
        Ec *running = current->next;
        running->cont = ret_user_sysexit;
        running->make_current();
        break;
    }
    default:
        printf("unknown syscall %d\n", number);
        break;
    };

    ret_user_sysexit();
}

bool allocate_pages(mword old_page, mword new_page)
{
    int num_pages = new_page - old_page;
    for (int i = num_pages; i > 0; --i)
    {
        void *alloced = Kalloc::allocator.alloc_page(1, Kalloc::FILL_0);
        if (!alloced)
        {
            printf("Alloc error.\n");
            free_pages(old_page + num_pages, old_page + i);
            return false;
        }
        mword phys = Kalloc::virt2phys(alloced);
        mword virt = (old_page + i) << 12;

        if (!Ptab::insert_mapping(virt, phys,
                                  Ptab::PRESENT | Ptab::RW | Ptab::USER))
        {
            printf("Mapping error.\n");
            Kalloc::allocator.free_page(alloced);
            free_pages(old_page + num_pages, old_page + i);
            return false;
        }
    }
    return true;
}

void free_pages(mword old_page, mword new_page)
{
    int num_pages = old_page - new_page;
    for (int i = num_pages; i > 0; --i)
    {
        mword virt = (new_page + i) << 12;
        mword phys = (Ptab::get_mapping(virt) & ~PAGE_MASK);
        void *kern_virt = Kalloc::phys2virt(phys);
        Kalloc::allocator.free_page(kern_virt);
        if (!Ptab::insert_mapping(virt, 0x0, 0))
            printf("Unmapping error.\n");
    }
}

void zero_area(mword old_break, mword old_page, mword size)
{
    mword phys = (Ptab::get_mapping(old_break) & ~PAGE_MASK) + old_break - (old_page << 12);
    void *kern_virt = Kalloc::phys2virt(phys);
    memset(kern_virt, 0, size);
}

Ec::Ec(mword start, mword stack)
{
    this->regs.ecx = stack;
    this->regs.edx = start;
    this->cont = reinterpret_cast<void (*)()>(start);
}
