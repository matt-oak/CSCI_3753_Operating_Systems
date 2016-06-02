

#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h>
#include<asm/uaccess.h>
#define BUFFER_SIZE 1024

static char device_buffer[BUFFER_SIZE];

/* Variable to keep track of the amount of times the device has been opened */
int oCounter = 0;

/* Variable to keep track of the amount of times the device has been closed */
int cCounter = 0;

ssize_t simple_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	/* Get the size of what we are to copy from the device_buffer */
	int sizeOfBuffer = strlen(device_buffer);

	/* If there's nothing in our device_buffer (*offset is 0), there's nothing to read so return 0 */
	if (sizeOfBuffer == 0){
		printk(KERN_ALERT "The device buffer is currently empty");
		return 0;
	}
	
	/* Copy the data from offset to the end of the buffer */
	printk(KERN_ALERT "Reading from device\n");
	copy_to_user(buffer, device_buffer, sizeOfBuffer);
	
	/* Print out the amount of bytes that is in the device buffer */
	printk(KERN_ALERT "Device has read %d bytes\n", sizeOfBuffer);
	
	return 0;
}

ssize_t simple_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	/* Get the size of the user space buffer that we are writing from */
	int sizeofDBuffer = strlen(device_buffer);
	
	/* If the user space buffer is empty, there's nothing to write to return 0 */
	if (length == 0){
		return 0;
	}
	//memset
	
	//int amount_to_write = BUFFER_SIZE - *offset;
	/* Set the offset to the current size of the device buffer so we don't overwrite what's currently there */
	*offset = sizeofDBuffer;

	/* Write from the user space buffer to the device buffer */
	printk(KERN_ALERT "Writing to device\n");
	copy_from_user(device_buffer + *offset, buffer, length);
	
	/* Print out the amount of bytes that the user wrote */
	printk(KERN_ALERT "Device has written %zu bytes", strlen(buffer));
	
	return length;
}


int simple_char_driver_open (struct inode *pinode, struct file *pfile)
{
	/* Print that the device has been opened */
	printk(KERN_ALERT "Device is OPEN\n");
	
	/* Increment oCounter, reflecting the number of times the device has been opened */
	oCounter++;
	
	/* Print the number of times the device has been opened */
	printk(KERN_ALERT "Device has been opened %d times\n", oCounter);
	
	return 0;
}


int simple_char_driver_close (struct inode *pinode, struct file *pfile)
{
	/* Print that the device has been closed */
	printk(KERN_ALERT "Device is CLOSED\n");
	
	/* Increment cCounter, reflecting the number of times the device has been closed */
	cCounter++;
	
	/* Print the number of times the device has been clsoed */
	printk(KERN_ALERT "Device has been closed %d times\n", cCounter);
	
	return 0;
}

struct file_operations simple_char_driver_file_operations = {

	.owner   = THIS_MODULE,
	/* Adding pointers to function corresponding the proper file operations we'd like to implement*/
	.open 	 = simple_char_driver_open,
	.release = simple_char_driver_close,
	.read	 = simple_char_driver_read,
	.write	 = simple_char_driver_write,
};

static int simple_char_driver_init(void)
{
	/* Printing to the log file that the init function has been called */
	printk(KERN_ALERT "Within %s function\n",__FUNCTION__);
	
	/* Registering the device */
	/* Assigning 420 as Major # for driver*/
	register_chrdev(420, "simple_char_driver", &simple_char_driver_file_operations);
	
	return 0;
}

static int simple_char_driver_exit(void)
{
	/* Printing to the log file that the exit function has been called */
	printk(KERN_ALERT "Within %s function\n",__FUNCTION__);
	
	/* Unregistering the device */
	unregister_chrdev(420, "simple_char_driver");
	
	return 0;
}

/* Have module_init point to our init function */
module_init(simple_char_driver_init);

/* Have module_exit point to our exit function */
module_exit(simple_char_driver_exit);
