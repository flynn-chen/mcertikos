#include <syscall.h>
#include <stdio.h>

int main() {
	int prevt=0;
	int t=0;
	int pid;
	int cpu;
	printf("Client 1, running\n");
	while(1) {
		t = gettime();
		pid = getpid();
		cpu = getcpu();
		if (1) {
	//	if ((t - prevt) >= 100) {//for qemu
	//	if ((t - prevt) >= 2) {//for simnow
			prevt = t;
			printf("Client 1, running%d\n", prevt);
			printf("Client 1, pid %d, cpu %d, I am still here\n", pid, cpu);
		}
	}
}
