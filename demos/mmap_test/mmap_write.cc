#include <stdio.h>

#include <stdlib.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <unistd.h>

#include <sys/mman.h>
#include <fcntl.h>

#include <cstring>
#include <iostream>
#include <string_view>

int main()
{
    std::cout << sizeof(unsigned long) << std::endl;
    int fd = open("test_file.dat4", O_RDWR | O_CREAT, (mode_t)0600);

    const char *text = "hello";

    size_t textsize = std::strlen(text) + 1;
    //auto size = 50*1024*1024*1024UL;
    auto size = 1200UL;

    auto p = lseek(fd, size-1, SEEK_SET);

    std::cout << p << std::endl;

    write(fd, "", 1);

    char *start = static_cast<char*>(mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    close(fd);

    memcpy(start+1024, "hello", 5);
    msync(start, size, MS_SYNC);
    munmap(start, size);

    std::cout << "verify..." << std::endl;


    char *buf = static_cast<char*>(malloc(5));

    int fd2 = open("test_file.dat4", O_RDONLY);
    p = lseek(fd2, 1024, SEEK_SET);
    std::cout << p << std::endl;
    read(fd2, buf, 5);

    std::cout << std::string_view(buf, 5) << std::endl;

    free(buf);

    return 0;
}
