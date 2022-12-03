#include <iostream>
#include <cmath>
#include <thread>
#include <future>
#include <functional>
#include <string>

#include <jemalloc/jemalloc.h>

// unique function to avoid disambiguating the std::pow overload set
int f(int x, int y) { return std::pow(x,y); }

/*
void task_lambda()
{
    std::packaged_task<int(int,int)> task([](int a, int b) {
        return std::pow(a, b);
    });
    std::future<int> result = task.get_future();

    task(2, 9);

    std::cout << "task_lambda:\t" << result.get() << '\n';
}

void task_bind()
{
    std::packaged_task<int()> task(std::bind(f, 2, 11));
    std::future<int> result = task.get_future();

    task();

    std::cout << "task_bind:\t" << result.get() << '\n';
}*/

void task_thread()
{
    std::packaged_task<int(int,int)> task(f);
    std::future<int> result = task.get_future();

    std::thread task_td(std::move(task), 2, 10);
    task_td.join();

    std::cout << "task_thread:\t" << result.get() << '\n';
}

void task_normal() {
    std::packaged_task<int(int,int)> task(f);
    std::future<int> result = task.get_future();

    std::function<void()> wrap_task = [&task](){task(2, 10);};

    wrap_task();
    std::cout << "task_normal:\t" << result.get() << '\n';
}

int main()
{
    //task_lambda();
    //task_bind();
    task_thread();
    task_normal();

    /*
    char* p_chars = new char[1024*1024*1024]{'\0'};
    char *p_char = new char{'a'};

    p_chars[1024] = 'b';
    std::cout << p_chars[1023] << p_chars[1024] << p_chars[1025] << std::endl;
    */

    /*
    std::string str;
    str.reserve(1024*1024*1024);
    std::cout << str.length() << std::endl;
    str.resize(10);
    std::cout << str.length() << std::endl;
    std::cout << str.capacity() << std::endl;
    */

    for (auto i = 0; i < 1000000; ++i) {
        void *buf = malloc(1024*1024*1024);
        free(buf);
    }
    std::cout << sizeof(double) << " " << sizeof(int) << std::endl;
    //char* p_chars = new char[1024*1024*1024];
}
