/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: ctor.cc
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 11-29-2022 09:54:24
 * @version $Revision$
 *
 **************************************************************************/

#include "ctor2.h"
#include "ctor1.h"

#include <iostream>
#include <memory>
#include <functional>
#include <algorithm>
#include <list>
#include <string>
#include <arpa/inet.h>

#include <sstream>
#include <atomic>

#include <zstd.h>


using namespace std;
using namespace std::literals;

class A {

    public:
    A() {}
    A(const A&) {};
    A(A&&) {};

    void func() { std::cout << "A::func"s << std::endl; }

    int i = 10;
};

template <typename... Funcs>
void var_func(Funcs... fns) {
    std::cout << sizeof...(fns) << std::endl;
    (fns(), ...);
}

int func3(int i) {
    std::cout << "func3: " << i << std::endl;
    return 1;
}

int func4(double d, float f) {

    std::cout << "func4: " << d << " " << f << std::endl;
    return 1;
}

int func5(char c) {

    std::cout << "func5: " << c << std::endl;
    return 1;
}

int func6(const char * buf, A& a) {

    a.i = 11;
    std::cout << "a.i = " << a.i << std::endl;
    return  a.i;
}

class B {
public:
    B() {}
    B(B&&) {}
};

std::list<B> list_b{};

const std::list<B>& return_list() {
    return list_b;
}


int main() {
#if defined(ZSTD_STATIC_LINKING_ONLY)
    std::cout << "hello zstd lib" << std::endl;
    ZSTD_threadpool *pool = ZSTD_createThreadPool(2);
    if (pool == nullptr) {
        std::cout << "thread pool is null" << std::endl;
    }
#endif

    std::cout << "ZSTD D in , out buff size: " << ZSTD_DStreamInSize() << " " << ZSTD_DStreamOutSize() << std::endl;
    std::cout << "ZSTD C in , out buff size: " << ZSTD_CStreamInSize() << " " << ZSTD_CStreamOutSize() << std::endl;

    if (auto r = -1; r) {
        std::cout << "hello -1" << std::endl;
    }

    ZSTD_DCtx *dctx;

    auto& l = return_list();

    atomic_ulong aul = 0;
    atomic<char*> apchar = nullptr;

    char ch = 'a';
    apchar = &ch;

    SPDLOG_INFO("atomic char pointer: {}", *(apchar.load()));

    A a;

    //A a2 = std::move(a);

    int arr[10]{1,2,3,4,5};

    spdlog::set_pattern("[tid %t] %+ ");
    spdlog::set_level(spdlog::level::info);
    auto console = spdlog::stdout_color_mt("test1");



    //auto p = make_shared<int>();

    //p = &arr[0];
    //

    int func(int);

    func(std::move(arr[3]));
    func1();
    func2();

    constexpr size_t str_arr_size = 500;
    auto p_str_arr = shared_ptr<std::string[]>(new std::string[str_arr_size]{"abcdefg", "hello"});

    std::cout << "diff: " << reinterpret_cast<char*>(&p_str_arr[1]) - reinterpret_cast<char*>(&p_str_arr[0]) << std::endl;

    std::cout << "htons: " << htons(-1) << std::endl;

    ostringstream oss;
    oss << "hello" << 123;

    cout << "oss size: " << oss.str().size() << endl;

    return 0;
}

int func(int) {
    auto console = spdlog::get("test1");
    console->info("hello world, {}, {}, {}, {}, {}", ZSTD_CStreamOutSize(), sizeof(int), sizeof(size_t), sizeof(uint32_t), sizeof(unsigned short));
    SPDLOG_INFO("hello world");
    SPDLOG_WARN("a test message.");
    spdlog::warn("a test message again.");

    const char* p = static_cast<char*>(malloc(1));
    //free(p);
    //
    auto p1 = const_cast<char*>(p);
    free(p1);

    auto fn3 = bind(func3, 5);
    auto fn4 = bind(func4, 1.5, 1.2);
    auto fn5 = bind(func5, 'c');

    A a;
    auto fn6 = std::mem_fn(&A::i);
    auto fn7 = [fn6, &a]{std::cout << "A::i = " << fn6(a) << std::endl;};

    var_func(fn3, fn4, fn5, fn7);

    //auto var_func2 = std::bind(var_func, fn3, fn4, fn5, fn7);

    auto pa = make_shared<A>();
    std::function<int()> func8 = bind(func6, p, *pa);
    func8();

    auto func9 = std::bind(&A::func, pa);
    func9();

    return 0;
}
