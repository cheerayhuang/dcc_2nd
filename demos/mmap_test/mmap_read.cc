#include <stdio.h>

#include <stdlib.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <unistd.h>

#include <sys/mman.h>
#include <fcntl.h>

int main()
{

    int fd = open("../build/data_test3M.arrow", O_RDONLY);

    struct stat statbuf;

    char *start;

    char buf[2] = {0};

    int ret = 0;

    fstat(fd, &statbuf);

    start = static_cast<char*>(mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0));

    do {

        *buf = start[ret++];

    }while(ret < statbuf.st_size);

    return 0;

}
