#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"

int i=0;

int main(int argc, char *argv[]) {
	// printf(1,"Number of free pages in before fork are : %d\n",getNumFreePages());
	// int i=0;
	if(fork()==0){
			printf(1,"Number of free pages in child are : %d\n",getNumFreePages());
			i++;
			printf(1,"Number of free pages in after writing child are : %d\n",getNumFreePages());
			exit();
	}
	wait();
	exit();
}

