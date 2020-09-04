#include <linux/module.h>

# include <linux/syscalls.h>

unsigned long ** get_sys_call_table_by_entry_SYSCALL_64(void) {
    u64 reg_lstar, i;
    void * sys_call_table = NULL;

    rdmsrl(MSR_LSTAR, reg_lstar);
    for (i = 0; i <= PAGE_SIZE; ++i) {
        u8 *tmp_addr = (u8 *) reg_lstar + i;
        if (tmp_addr[0] == 0xff && tmp_addr[1] == 0x14 && tmp_addr[2] == 0xc5) {
            sys_call_table = tmp_addr + 3;
            return (void *)(0xffffffff00000000 | *(u32 *)sys_call_table);
        }
    }
    return NULL;
}

unsigned long ** get_sys_call_table_by_sys_close(void) {
    unsigned long **sys_call_table = (unsigned long **)PAGE_OFFSET; //PAGE_OFFSET is the start of the kernel address
    while ((unsigned long)sys_call_table < ULONG_MAX) {
        if (sys_call_table[__NR_close] == (unsigned long *)sys_close) {
            return sys_call_table;
        }
    sys_call_table ++;
    }
    return NULL;
}

unsigned long ** get_sys_call_table(void) {
    return get_sys_call_table_by_entry_SYSCALL_64();
    //return get_sys_call_table_by_sys_close();
}

void disable_write_protection(void) {
    unsigned long reg_cr0 = read_cr0();
    clear_bit(16, &reg_cr0);
    write_cr0(reg_cr0);
}

void enable_write_protection(void) {
    unsigned long reg_cr0 = read_cr0();
    set_bit(16, &reg_cr0);
    write_cr0(reg_cr0);
}

