#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
	char input;
	char buffer[1024];
	int file = open("/dev/simple_char_driver", O_RDWR);
	
	printf("Press 'r' to read from device\n");
	printf("Press 'w' to write to device\n");
	printf("Press 'e' to exit from the device\n");
	printf("Press anything else to keep reading or writing to/from the device\n");
	printf("Input: ");
	scanf("%c", &input);
	
	while(1){
		if (input == 'r'){
			read(file, buffer, BUFFER_SIZE);
			printf("Device reads: %s\n", buffer);
			printf("Press 'r' to read from device\n");
			printf("Press 'w' to write to device\n");
			printf("Press 'e' to exit from the device\n");
			printf("Press anything else to keep reading or writing to/from the device\n");
			printf("Input: ");
		}
		else if (input == 'w'){
			printf("Enter what you want to write to the device: ");
			scanf("%s", buffer);
			write(file, buffer, BUFFER_SIZE);
			printf("Press 'r' to read from device\n");
			printf("Press 'w' to write to device\n");
			printf("Press 'e' to exit from the device\n");
			printf("Press anything else to keep reading or writing to/from the device\n");
			printf("Input: ");
		}
		else if (input == 'e'){
			return 0;
		}
		scanf("%c", &input);
	}
	close(file);
	return 0;
}
	
	
	
