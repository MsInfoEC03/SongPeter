#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/moduleparam.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#define GLOBALMEM_SIZE 0x1000
#define MEM_CLEAR 0x1
#define GLOBALMEM_MARJOR 230
#define device_name "mymap"

static unsigned char array[]={"My name is mmap test."};
static unsigned char *buffer;

static spinlock_t lock;
struct semaphore sem;

static int globalmem_major = GLOBALMEM_MARJOR;
module_param(globalmem_major,int,S_IRUGO);

struct globalmem_dev{
	struct cdev cdev;
	unsigned char mem[GLOBALMEM_SIZE];
};


struct globalmem_dev* globalmem_devp;

static ssize_t globalmem_read(struct file* filp,char __user * buf,size_t size,loff_t* ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalmem_dev* dev = filp->private_data;
	
	if(p >= GLOBALMEM_SIZE)
		return 0;
	if(count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;
	
	if(copy_to_user(buf,dev->mem + p,count)){
		ret = -EFAULT;
	}else{
		*ppos += count;
		ret = count;
		
		printk(KERN_INFO "read %u bytes(s) from %lu\n",count,p);
	}
	return ret;
}

static ssize_t globalmem_write(struct file* filp,const char __user * buf,size_t size,loff_t* ppos)
{
	unsigned long p = *ppos;
	unsigned int count = size;
	int ret = 0;
	struct globalmem_dev* dev = filp->private_data;
	
	if(p >= GLOBALMEM_SIZE)
		return 0;
	if(count > GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;
	
	if(copy_from_user(dev->mem + p,buf,count))
		ret = -EFAULT;
	else{
		*ppos += count;
		ret = count;
		
		printk(KERN_INFO "written %u bytes(s) from %lu\n",count,p);
	}
	
	return ret;
}

static loff_t globalmem_llseek(struct file* filp,loff_t offset,int orig)
{
	loff_t ret = 0;
	switch(orig){
	case 0:/*从文件头位置seek*/
		if(offset < 0){
			ret = -EINVAL;
			break;
		}
		if((unsigned int)offset > GLOBALMEM_SIZE){
			ret = -EINVAL;
			break;
		}
		filp->f_pos = (unsigned int)offset;
		ret = filp->f_pos;
		break;
	case 1:/*从文件当前位置开始seek*/
		if((filp->f_pos + offset) > GLOBALMEM_SIZE){
			ret = -EINVAL;
			break;
		}
		filp->f_pos += offset;
		ret = filp->f_pos;
		break;
	default:
		ret = EINVAL;
		break;
	}
	return ret;
}

static long globalmem_ioctl(struct file* filp,unsigned int cmd,unsigned long arg)
{
//	struct globalmem_dev* dev = filp->private_data;
	
	switch(cmd){
	case MEM_CLEAR:
//		memset:(dev->mem,0,GLOBALMEM_SIZE);
		printk(KERN_INFO "globalmem is set to zero\n");
		break;
	default:
		return -EINVAL;
	}
	
	return 0;
}

static int globalmem_open(struct inode* inode,struct file* filp)
{
	filp->private_data = globalmem_devp;
	return 0;
}

static int globalmem_release(struct inode* inode,struct file* filp)
{
	return 0;
}

static int my_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long page;
	unsigned char i;
	unsigned long start = (unsigned long)vma->vm_start;
	unsigned long size  =(unsigned long)(vma->vm_end - vma->vm_start);
	page = virt_to_phys(buffer);
	if(remap_pfn_range(vma,start,page>>PAGE_SHIFT,size,PAGE_SHARED))
	return -1;
down(&sem);
spin_lock(&lock);
	for(i=0; i<21; i++)
	buffer[i] = array[i];
up(&sem);
spin_unlock(&lock);	
	return 0;
}



static const struct file_operations globalmem_fops = {
	.owner = THIS_MODULE,

	.mmap = my_mmap,
	.llseek = globalmem_llseek,
	.read = globalmem_read,
	.write = globalmem_write,
	.unlocked_ioctl = globalmem_ioctl,
	.open = globalmem_open,
	.release = globalmem_release,
};

static struct miscdevice misc =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name  = device_name,
	.fops  = &globalmem_fops,
};
static void __exit globalmem_exit(void)
{
	cdev_del(&globalmem_devp->cdev);
	kfree(globalmem_devp);
	misc_deregister(&misc);
	ClearPageReserved(virt_to_page(buffer));
	
	unregister_chrdev_region(MKDEV(globalmem_major,0),1);
}
module_exit(globalmem_exit);

static void globalmem_setup_cdev(struct globalmem_dev* dev,int index)
{
	int err,devno = MKDEV(globalmem_major,index);

	cdev_init(&dev->cdev,&globalmem_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev,devno,1);
	if(err)
		printk(KERN_NOTICE "Error %d adding globalmem%d",err,index);
}

static int __init globalmem_init(void)
{
	int ret;
	dev_t devno = MKDEV(globalmem_major,0);
	
	if(globalmem_major)
		ret = register_chrdev_region(devno,1,"globalmem");
	else{
		ret = alloc_chrdev_region(&devno,0,1,"globalmem");
	}
	if(ret < 0)
		return ret;
	
	spin_lock_init(&lock);
	sema_init(&sem,1);
	misc_register(&misc);
	
	buffer = (unsigned char *)kmalloc(PAGE_SIZE,GFP_KERNEL);
	SetPageReserved(virt_to_page(buffer));

	globalmem_devp = kzalloc(sizeof(struct globalmem_dev),GFP_KERNEL);
	if(!globalmem_devp){
		ret = -ENOMEM;
		goto fail_malloc;
	}
	
	globalmem_setup_cdev(globalmem_devp,0);
	return 0;
	
	fail_malloc:
	unregister_chrdev_region(devno,1);
	return ret;
}
module_init(globalmem_init);

MODULE_LICENSE("GPL v2");
