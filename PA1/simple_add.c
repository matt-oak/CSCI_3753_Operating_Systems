#include <linux/kernel.h>
#include <linux/linkage.h>

asmlinkage long sys_simple_add(int number1, int number2, int *result){
  //Print out the first parameter
  printk(KERN_EMERG "Number1 = %d\n", number1);

  //Print out the second parameter
  printk(KERN_EMERG "Number2 = %d\n", number2);

  //Dereference result and set it equal to number1 + number2
  *result = number1 + number2;

  //Print out the result
  printk(KERN_EMERG "Number1 + Number2 = %d\n", *result);
  return 0;
}
