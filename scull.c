#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>

#include "scull.h"

//#define DEBUG
#define NUM 4
#define DEVNAME "scull"

//DEFINE_SEMAPHORE(sem);

dev_t scull_devnum;
uint scull_major,scull_minor;
struct scull_dev scull_dev;

static void scull_trim(struct scull_dev *dev)
{
	int i;
	struct scull_qset *dptr ,*next;

	dptr = dev->data;
	for (; dptr != NULL ;dptr = next) {
		if (dptr->data) {
			for (i = 0 ;i < dev->qset ;i++) {
				kfree(dptr->data[i]);
			}
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}
	dev->qset = SCULL_QSET;
	dev->quantum = SCULL_QUANTUM;
	dev->data = NULL;
	dev->size = 0;
}

static struct scull_qset *scull_follow(struct scull_dev *dev,unsigned int item)
{
	struct scull_qset *dptr = dev->data;

	if (!dptr) {
		dptr = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		if (!dptr) {
			printk(KERN_DEBUG "kmalloc failed\n");
			return NULL;
		}
		memset(dptr, 0, sizeof(struct scull_qset));
	}
	while(item--) {
		if (!dptr->next) {
			dptr->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
			if (!dptr->next) {
				printk(KERN_DEBUG "kmalloc failed\n");
				return NULL;
			}
			memset(dptr->data, 0, sizeof(struct scull_qset));
		}
		dptr = dptr->next;
		continue;
	}
	return dptr;
}

#ifdef DEBUG
static void scull_mem_debug(struct scull_dev *dev)
{
	int i;
	struct scull_qset *dptr,*next;
		
	for (dptr = dev->data; dptr ;dptr = next) {
		if ((char *)dptr->data) {
			for (i = 0; i < dev->qset ;i++) {
				if ((char *)dptr->data[i]) {
					printk(KERN_DEBUG "[i]: %s\n",(char *)dptr->data[i]);
				}
			}
		}
		next = dptr->next;
		continue;
	}
}
#endif

static int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev;

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev;
	
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible(&dev->sem))
		    return -ERESTARTSYS;
		scull_trim(dev);
		up(&dev->sem);
	}
	return 0;
}

static ssize_t scull_read(struct file *filp, char __user *buffer, size_t count, loff_t *f_ops)
{
	struct scull_dev *dev;
	struct scull_qset *dptr;
	unsigned long itemsize;
	unsigned int qset,quantum;
	unsigned int s_pos,q_pos,reset,item;

	ssize_t retval = 0;

	dev = filp->private_data;
	qset = dev->qset;
	quantum = dev->quantum;
	itemsize = qset * quantum;

	if (down_interruptible(&dev->sem))
	    return -ERESTARTSYS;
	
	if (*f_ops > dev->size) {
		printk(KERN_DEBUG "can not read beyond data length\n");
		goto out;
	}
	if (*f_ops + count > dev->size) {
		count = dev->size - *f_ops;
	}
	
	item = *f_ops / itemsize;
	reset = *f_ops % itemsize;
	s_pos = reset / quantum;
	q_pos = reset % quantum;

#ifdef DEBUG
	scull_mem_debug(dev);
#endif
	dptr = scull_follow(dev,item);
	if (!dptr || !dptr->data || !dptr->data[s_pos]) {
		goto out;
	}
	if (count > quantum - q_pos) {
		count = quantum - q_pos;
	}
	if(copy_to_user(buffer, dptr->data[s_pos] + q_pos, count)) {
		printk(KERN_DEBUG "copy_to_user failed\n");
		goto out;
	}
	*f_ops += count;
	retval = count;
out:
	up(&dev->sem);
	return retval;
}

static ssize_t scull_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_ops)
{
	struct scull_dev *dev;
	struct scull_qset *dptr;
	unsigned long itemsize;
	unsigned int qset,quantum;
	unsigned int s_pos,q_pos,reset,item;

	ssize_t retval = -ENOMEM;

	dev = filp->private_data;
	qset = dev->qset;
	quantum = dev->quantum;
	itemsize = qset * quantum;

	if (down_interruptible(&dev->sem))
	    return -ERESTARTSYS;

	item = *f_ops / itemsize;
	reset = *f_ops % itemsize;
	s_pos = reset / quantum;
	q_pos = reset % quantum;

	dptr = scull_follow(dev, item);
	if (!dptr)
		goto out;
	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (!dptr->data)
			goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data[s_pos])
			goto out;
		memset(dptr->data[s_pos], '\0', quantum);
	}
	if (count > quantum - q_pos)
		count = quantum - q_pos;
	if (copy_from_user(dptr->data[s_pos] + q_pos, buffer, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_ops += count;
	retval = count;
	
	if (dev->size < *f_ops)
		dev->size = *f_ops;
#ifdef DEBUG
	scull_mem_debug(dev);
#endif
out:
	up(&dev->sem);
	return retval;
}

static long scull_ioctl(struct file *filp, unsigned int command, unsigned long arg)
{

	return (long)0;
}

static loff_t scull_llseek(struct file *filp, loff_t offset, int where)
{
	return 0;
}

static int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct file_operations scull_ops = {
	.owner		    = THIS_MODULE,
	.open		    = scull_open,
	.read		    = scull_read,
	.write		    = scull_write,
	.unlocked_ioctl	    = scull_ioctl,
	.llseek		    = scull_llseek,
	.release	    = scull_release,
};

static int scull_init_dev(void)
{
	int ret;
	if (scull_major) {
		ret = register_chrdev_region(MKDEV(scull_major,0), 1, DEVNAME);
		scull_devnum = MKDEV(scull_major,0);
	} else {
		ret = alloc_chrdev_region(&scull_devnum, 0, 1, DEVNAME);
		scull_major = MAJOR(scull_devnum);
		scull_minor = MINOR(scull_devnum);
	}

	if (ret)
		printk(KERN_ERR "register device failed\n");

	printk(KERN_DEBUG "register_chrdev_region success\n");
	return ret;
}

static void scull_setup_dev(struct scull_dev *dev)
{
	if (scull_init_dev()) {
		printk(KERN_DEBUG "scull_init_dev failed\n");
		unregister_chrdev_region(scull_devnum, 1);
	}
	dev->data = NULL;
	dev->qset = SCULL_QSET;
	dev->quantum = SCULL_QUANTUM;
	dev->size = 0;
	sema_init(&dev->sem, 1);
	cdev_init(&dev->cdev, &scull_ops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_ops;
	if (cdev_add(&dev->cdev, scull_devnum, 1)) {
		printk(KERN_DEBUG "cdev add failed\n");
		unregister_chrdev_region(scull_devnum, 1);
	}
	printk(KERN_DEBUG "cdev_add success\n");
}

static int __init scull_init(void)
{
	scull_major = SCULL_INIT;
	scull_minor = SCULL_INIT;
	memset(&scull_dev,0,sizeof(struct scull_dev));
	scull_setup_dev(&scull_dev);
	printk(KERN_DEBUG "scull driver init\n");
	return 0;
}

static void __exit scull_exit(void)
{
	scull_trim(&scull_dev);
	cdev_del(&scull_dev.cdev);	
	unregister_chrdev_region(scull_devnum, 1);
	printk(KERN_DEBUG "scull driver exit\n");
}
MODULE_AUTHOR("joker");
MODULE_LICENSE("GPL v2");
module_init(scull_init);
module_exit(scull_exit);
