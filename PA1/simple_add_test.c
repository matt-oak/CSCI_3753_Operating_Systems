#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(){
	//Allocate an integer on the stack
	int intAlloc;
	
	//Declare a pointer, equal to the address of the integer we just allocated
	int *result = &intAlloc;
	
	//Perform the simple_add system call (ID #324) and supply it with
	//2 integers (2 and 3) as well as the address that we want to store
	//the result in
	syscall(324, 2, 3, result);
	
	//Print out the equation along with the dereferenced summation
	printf("2 + 3 = %d", *result);
	return 0;
}
