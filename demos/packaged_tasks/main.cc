/**************************************************************************
 *
 * Copyright (c) 2015 Cheeray Huang. All Rights Reserved
 *
 * @file: main.cc
 * @author: Huang Qiyu
 * @email: cheeray.huang@gmail.com
 * @date: 11-24-2022 18:18:19
 * @version $Revision$
 *
 **************************************************************************/

#include <future>
#include <thread>
#include <functional>

#include <iostream>
#include <cmath>

double func(int i, int f) {
    return static_cast<double>(i + f);
}


int f(int x, int y) { return std::pow(x,y); }

void task_normal() {
    std::packaged_task<int(int,int)> task(f);
    std::future<int> result = task.get_future();

    task(2, 10);
    std::cout << "task_normal:\t" << result.get() << '\n';
}

int main() {

    task_normal();

    return 0;
}
