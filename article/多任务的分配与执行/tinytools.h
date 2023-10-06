#pragma once

#include <thread>
#include <chrono>
#include <memory>
#include <future>
#include <functional>
#include <iostream>
#include <queue>
#include "logger.h"

class tinytools
{
public:
    template<class T, class... Args>
    static auto measureTask(T&& func,bool logfile, Args&&... args)->std::future<typename std::result_of<T(Args...)>::type>
    {
        using return_type = typename std::result_of<T(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>
            (std::bind(std::forward<T>(func), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        auto begin = std::chrono::high_resolution_clock::now();
        (*task)();
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
        if(!logfile)  printf("Ö´ÐÐÊ±¼ä: % .3f seconds.\n", elapsed.count() * 1e-9);
       // else quickLog(const char* istr)
        return res;
    }

};
