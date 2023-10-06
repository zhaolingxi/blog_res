#include <iostream>
#include <vector>
#include <thread>
#include "lockless_queue_inlist.h"
#include"tinytools.h"
#include"multi_thread.h"

using namespace std;
#define TESTNUM 1000
void AlterPrint() {
    int sum = 0;
    std::atomic_bool flag = false;//标记位置，用于解决线程竞争问题

    std::thread t1([&]() {
        while (true) {
            if (!flag.load()) {
                if (sum == TESTNUM) {
                    flag.store(true);
                    break;
                }
                ++sum;
                flag.store(true);
            }
        }});
    std::thread t2([&]() {
        while (true) {
            if (flag.load()) {
                if (sum == TESTNUM) {
                    flag.store(false);
                    break;
                }
                ++sum;
                flag.store(false);
            }
        }
        });
    t1.join();
    t2.join();
}

void add(int a, int b)
{
    int c = a + b;
    return;
}



int main() {
    //mutex m;
    NoLockQueue<int> freelockque;//无锁队列（基于链表实现）
    multi_thread* pmulti_th = multi_thread::getInstance();//线程池
    pmulti_th->addThread(16);
    pmulti_th->commitTask(add, 2, 3);

    int* test_int = new int[TESTNUM];
    std::atomic_bool flag = false;//标记位置，用于解决线程竞争问题

    auto singletest = [&pmulti_th]() {//简单加法计算
        int testnum = TESTNUM;//任务总数
        while (testnum--)
        {
            pmulti_th->commitTask(add, 1, 2);
        }
    };

    auto singletest2 = [&freelockque]() {//内存读取测试，向内存中写入int
        int testnum = TESTNUM;//任务总数
        while (testnum--)
        {
            int i = 1;
            freelockque.push(i);
        }
    };

    auto singletest3 = [&freelockque]() {//内存读取测试，向内存中读出int
        int testnum = TESTNUM;//任务总数
        while (testnum--)
        {
            int i;
            freelockque.pop(i);
        }
    };

    auto singletest4 = [&flag,&test_int](int &ipos) {//内存读取测试，向内存中读出int(单次)
        while (true) {
            if (flag.load())
            {
                int tmp = test_int[ipos];
                flag.store(false);
                cout << "touch singletest4" << endl;
                break;
            }
        }
    };

    auto singletest5 = [&flag, &test_int](int& ipos) {//内存读取测试，向内存中取出int(单次)
        while (true) {
            if (!flag.load())
            {
                test_int[ipos]=1;
                flag.store(true);
                cout << "touch singletest5" << endl;
                break;
            }
        }
    };

    //{
    //    lock_guard<mutex> lm(m);    
    //    tinytools::measureTask(singletest,0);
    //}

    //数据读写测试(无锁队列)
    //{
    //    lock_guard<mutex> lm(m);
    //    auto it = [&]() {
    //        thread th1(singletest2);
    //        thread th2(singletest3);
    //        th1.join();
    //        th2.join();
    //    };
    //   tinytools::measureTask(it, 0);
    //}

    //数据读写测试(线程池)
    {
        //lock_guard<mutex> lm(m);
        auto it2 = [&]() {
            //thread1添加写入任务
            thread th1([&pmulti_th,&singletest4]() {
                int testnum = TESTNUM;
                while (testnum--)
                {
                    pmulti_th->commitTask(singletest4, testnum);
                }
                } );
            //thread1添加读取任务
            thread th2([&pmulti_th, &singletest5]() {
                int testnum = TESTNUM;
                while (testnum--)
                {
                    pmulti_th->commitTask(singletest5, testnum);
                }
                });
            th1.join();
            th2.join();
            
        };
        tinytools::measureTask(it2, 0);
      
    }
    
    getchar();

    return 0;
}