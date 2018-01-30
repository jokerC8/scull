#ifndef _SCULL_H
#define _SCULL_H

#include <linux/cdev.h>
#include <linux/kernel.h>
//#include <linux/semaphore.h>
#include <uapi/asm-generic/fcntl.h>

#define SCULL_INIT 0
#define SCULL_QSET 2000
#define SCULL_QUANTUM 512

struct scull_qset {
	void **data;
	struct scull_qset *next;
};

struct scull_dev {
	struct scull_qset *data;
	int quantum;
	int qset;
	unsigned long size;
	unsigned int access_key;
//	struct semaphore sem;
	struct cdev cdev;
};
#endif
