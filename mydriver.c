#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/fdtable.h>
#include <linux/rcupdate.h>
#include <linux/fs_struct.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "mydriver_ioctls.h"

#define DEV_NAME "mydriver"

/* Path to File: /proc/procfiles */
#define PROC_NAME "procfiles"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Artur Baruchi & Edson Midorikawa");
MODULE_DESCRIPTION("Get open files from a given process - IOCTL");

struct mydriver_t
{
  struct cdev *cdev;
  uint8_t major;
  uint8_t minor;
};

static struct mydriver_t my_mydriver = {
  .cdev = 0,
  .major = 0,
  .minor = 0,
};

//Struct to Store Process Information
struct list_mypid {
        int mypid;
        char comm;
        struct files_struct *myfiles;
        struct list_head mylist;
};

static LIST_HEAD(plisthead);
int MY_PID;

/**** Function Prototypes ****/
int getPID (int parent);
int cleanLIST (void);
static long mydriver_ioctl(struct file *f, unsigned int cmd, unsigned long ioctl_param);
static inline int mydriver_create_dev(void);
static inline void mydriver_release_dev(void);
/*****************************/


/* proc functions */
/**** Begin ****/
static void *file_proc_start (struct seq_file *m, loff_t *pos) {

   struct list_head *ret;

   if(*pos == NULL)
                *pos=1;
   ret = seq_list_start_head(&plisthead, *pos);
   if (ret == NULL)
                cleanLIST();
}

static void *file_proc_next (struct seq_file *p, void *v, loff_t *pos) {
   struct list_head *ret;
   ret = seq_list_next(v, &plisthead, pos);
   return ret;
}

static int file_proc_show (struct seq_file *m, void *v) {
   struct list_mypid *f = list_entry(v, struct list_mypid, mylist);

  struct files_struct *current_files;
  struct fdtable *files_table;
  struct path files_path;
  char *cwd;
  char *buf = (char *)kmalloc(GFP_KERNEL,100*sizeof(char));
  int i=0;

  current_files = f->myfiles;
  files_table = files_fdtable(current_files);
  while(files_table->fd[i] != NULL) {
        files_path = files_table->fd[i]->f_path;
        cwd = d_path(&files_path,buf,100*sizeof(char));
        seq_printf(m,"%s\n",cwd);
        i++;
  }
   kfree (buf);
   return 0;
}

static void file_proc_stop (struct seq_file *m, void *v) {

}

static struct seq_operations file_seq_ops = {
   .start = file_proc_start,
   .next = file_proc_next,
   .stop = file_proc_stop,
   .show = file_proc_show
};

static int file_open(struct inode *inode, struct file *file) {
   cleanLIST();
   getPID(MY_PID);
   return seq_open(file, &file_seq_ops);
};

static struct file_operations proc_file_ops = {
   .owner = THIS_MODULE,
   .open = file_open,
   .read = seq_read,
   .llseek = seq_lseek,
   .release = seq_release
};

/**** End ****/

//Build Process List
int getPID (int parent) {
        struct list_mypid *temp, *pos;
        int childPID;

        struct task_struct *task, *child_task;
        struct list_head *children_list;

        for_each_process(task) {
                if (parent == task->pid) {
                        //Add the parent Process
                        temp = kmalloc(sizeof(struct list_mypid), GFP_KERNEL);
                        temp->mypid=task->pid;
                        temp->comm=task->comm;
                        temp->myfiles=task->files;
                        list_add(&temp->mylist,&plisthead);

                        list_for_each(children_list,&task->children) {
                                child_task=list_entry(children_list, struct task_struct, sibling);
                                if (child_task->pid!=NULL)
                                        getPID(child_task->pid);
                        }
                }
        }
        return 0;
}

//Cleanup the List
int cleanLIST (void) {
        struct list_head *pos, *q;
        struct list_mypid *temp;

        list_for_each_safe(pos,q,&plisthead) {
                temp=list_entry(pos,struct list_mypid,mylist);
                list_del(pos);
                kfree(temp);
        }
        return 0;
}


static long mydriver_ioctl(struct file *f, unsigned int cmd, unsigned long ioctl_param)
{

  int pid;
  struct list_mypid *t;
  struct list_head *children_list;

  //Open Files related variables
  struct files_struct *current_files;
  struct fdtable *files_table;
  struct path files_path;
  char *cwd;
  char *buf = (char *)kmalloc(GFP_KERNEL,100*sizeof(char));
  int i=0;


  if (cmd == MYDRIVER_IOCTL_CMD1)
  {
    printk(KERN_INFO "Hello ");
  }
  else if (cmd == MYDRIVER_IOCTL_CMD2)
  {
    printk(KERN_INFO "IOCTL ");
  }
  else if (cmd == MYDRIVER_IOCTL_CMD3)
  {
    printk(KERN_INFO "World!\n");
  }
  else if (cmd == MYDRIVER_IOCTL_CMD4)
  {
    pid = ioctl_param;
    cleanLIST();
    getPID(pid);
    MY_PID=pid;

    printk(KERN_INFO "-- IOCTL RECEIVED --\n");
    list_for_each_entry(t,&plisthead,mylist) {
        printk(KERN_INFO "PID: %d\n",t->mypid);

        current_files = t->myfiles;
        files_table = files_fdtable(current_files);
        while(files_table->fd[i] != NULL) {
                files_path = files_table->fd[i]->f_path;
                cwd = d_path(&files_path,buf,100*sizeof(char));
                printk(KERN_INFO "Open File %s\n",cwd);
                i++;
        }
    }
   }

  return 0;
}

static struct file_operations mydriver_fops =
{
  owner: THIS_MODULE,
  unlocked_ioctl: mydriver_ioctl,
};


static inline int mydriver_create_dev(void)
{
  struct mydriver_t *driver = &my_mydriver;
  struct cdev *cdev;
  dev_t devt;
  int rc;

  rc = alloc_chrdev_region(&devt, driver->minor, 1, DEV_NAME);
  driver->major = MAJOR(devt);

  if (rc)
  {
    printk(KERN_ERR "Failed to register chrdev region with major %d.\n", driver->major);
    goto err_region;
  }

  // register cdev
  rc = -ENOMEM;
  cdev = cdev_alloc();
  if (!cdev)
    goto err_cdev;

  cdev->owner = THIS_MODULE;
  cdev->ops = &mydriver_fops;

  rc = cdev_add(cdev, devt, 1);
  if (rc)
    goto err_cdev;

  driver->cdev = cdev;

  printk( "Driver allocated major:%d minor:%d\n", MAJOR(devt), MINOR(devt));
  return 0;

err_cdev:
  cdev_del(cdev);
err_region:
  unregister_chrdev_region(devt, 1);
  return -EFAULT;
}

//Release Char
static inline void mydriver_release_dev(void)
{
  struct mydriver_t *driver = &my_mydriver;
  cleanLIST();

  if (driver->cdev)
  {
    unregister_chrdev_region(driver->cdev->dev, 1);
    cdev_del(driver->cdev);
  }
}

static int __init mydriver_init(void)
{
        struct proc_dir_entry *entry;
        entry = create_proc_entry(PROC_NAME, 0, NULL);
        if (entry) {
                entry->proc_fops = &proc_file_ops;
        }

        return mydriver_create_dev();
}

static void __exit mydriver_exit(void)
{
        remove_proc_entry(PROC_NAME, NULL);
        mydriver_release_dev();
}

module_init(mydriver_init);
module_exit(mydriver_exit);
