#include<stdio.h>
#include<sys/mman.h>
#include<unistd.h>
#include<string.h>
int main(int argc,const char* argv[]){
    void* mem = mmap(NULL,4096,PROT_READ | PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(mem==MAP_FAILED){
        printf("Mem Allocation Failed");
        return 1;
    }

    strcpy((char*)mem,"Hello from mmap");
    *((char*)mem+1) = 'E';
    printf("%s",(char*)mem);
         

    munmap(mem,4096);
    return 0;
}