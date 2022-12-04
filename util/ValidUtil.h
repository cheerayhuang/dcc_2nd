#pragma once
#include <memory>
#include <string>
#include <stdexcept>

#include "ArrowUtil.h"

template <typename ...Args>
std::string string_format(const std::string& format, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;

    auto size = static_cast<size_t>(size_s);

    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);

    return std::string( buf. get(), buf.get()+size-1);
}


class ValidUtil {
public:
    static std::string getValidResult(std::string strings1[], int ints[], double doubles[], std::string strings2[], long total) {
        return "strings1: " + ValidStringArray(strings1, total) +
                ", ints: " + ValidIntArray(ints, total) +
                ", doubles: " + ValidDoubleArray(doubles, total) +
                ", strings2: " + ValidStringArray(strings2, total);
    }

    static std::string getValidResult(const std::string &filePath, int fileRows) {

        auto colValue = ArrowUtil::readDataFromFile(filePath, fileRows);

        return ValidUtil::getValidResult(
            colValue->stringVal1.get(),
            colValue->intVal.get(),
            colValue->doubles.get(),
            colValue->stringVal2.get(),
            colValue->total
        );
    }


private:

    static const int STEP = 100000;

    static std::string ValidIntArray(const int ints[], long total) {
        int sum = 0;
        long count = 0;

        for (int i = 0; i < total; i += STEP) {
            sum += ints[i];
            ++count;
        }

        if (count == 0) {
            return "0.0000";
        }

        return string_format("%.4f", (double) sum / count);
    }

    static std::string ValidStringArray(const std::string strings[], long total) {
        int sum = 0;
        long count = 0;

        for (int i = 0; i < total; i += STEP) {
            sum += strings[i].length();
            ++count;
        }

        if (count == 0) {
            return "0.0000";
        }

        return string_format("%.4f", (double) sum / count);

    }

    static std::string ValidDoubleArray(const double doubles[], long total) {
        double sum = 0;
        long count = 0;

        for (int i = 0; i < total; i += STEP) {
            sum += doubles[i];
            ++count;
        }

        if (count == 0) {
            return "0.0000";
        }

        return string_format("%.4f", (double) sum / count);
    }

};
