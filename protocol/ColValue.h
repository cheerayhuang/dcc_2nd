#pragma once

#include <string>
#include <memory>

struct ColValue {
    long total;

    std::shared_ptr<std::string[]> stringVal1;
    std::shared_ptr<int[]> intVal;
    std::shared_ptr<double[]> doubles;
    std::shared_ptr<std::string[]> stringVal2;

};

// '[index: 2bytes]#[len: 4bytes]#c#[datas...]'
//

struct Payload {
    int len;
    char *data;

};

