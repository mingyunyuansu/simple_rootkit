#include <linux/module.h>
#include <linux/kernel.h>

#include "../implementation/func.h"

unsigned long **sys_call_table;

int my_init(void) {
    printk("Start getting syscall table.\n");
    sys_call_table = get_sys_call_table();
    if (!sys_call_table) {
        printk("Failed to get syscall table.\n");
        return 0;
    }
    //Debug mesg
    printk("sys_call_table %p\n", sys_call_table);
    return 0;
}

void my_exit(void) {
    printk("Exiting.\n");
    return;
}

module_init(my_init);
module_exit(my_exit);
