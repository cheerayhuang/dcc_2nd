#pragma once

#include <random>

class RandomUtil {
public:
    static std::string randomString(int length) {
        return randomString("abcdefghijklmnopqrstuvwxyz0123456789", length);
    }

    static int randomInt(int min, int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(min, max);

        return distr(gen);
    }

    static double randomDouble(int min, int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> distr(min, max);

        return distr(gen);
    }

private:
    static std::string randomString(std::string baseString, int length) {
        if (baseString.empty()) {
            return "";
        }

        std::string tmp_s;
        tmp_s.reserve(length);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distr(0, (int)baseString.length()-1);


        for(int i = 0; i < length; ++i) {
            tmp_s += baseString[distr(gen)];
        }

        return tmp_s;
    }
};
