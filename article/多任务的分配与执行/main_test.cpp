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
    std::atomic_bool flag = false;//���λ�ã����ڽ���߳̾�������

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
    NoLockQueue<int> freelockque;//�������У���������ʵ�֣�
    multi_thread* pmulti_th = multi_thread::getInstance();//�̳߳�
    pmulti_th->addThread(16);
    pmulti_th->commitTask(add, 2, 3);

    int* test_int = new int[TESTNUM];
    std::atomic_bool flag = false;//���λ�ã����ڽ���߳̾�������

    auto singletest = [&pmulti_th]() {//�򵥼ӷ�����
        int testnum = TESTNUM;//��������
        while (testnum--)
        {
            pmulti_th->commitTask(add, 1, 2);
        }
    };

    auto singletest2 = [&freelockque]() {//�ڴ��ȡ���ԣ����ڴ���д��int
        int testnum = TESTNUM;//��������
        while (testnum--)
        {
            int i = 1;
            freelockque.push(i);
        }
    };

    auto singletest3 = [&freelockque]() {//�ڴ��ȡ���ԣ����ڴ��ж���int
        int testnum = TESTNUM;//��������
        while (testnum--)
        {
            int i;
            freelockque.pop(i);
        }
    };

    auto singletest4 = [&flag,&test_int](int &ipos) {//�ڴ��ȡ���ԣ����ڴ��ж���int(����)
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

    auto singletest5 = [&flag, &test_int](int& ipos) {//�ڴ��ȡ���ԣ����ڴ���ȡ��int(����)
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

    //���ݶ�д����(��������)
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

    //���ݶ�д����(�̳߳�)
    {
        //lock_guard<mutex> lm(m);
        auto it2 = [&]() {
            //thread1���д������
            thread th1([&pmulti_th,&singletest4]() {
                int testnum = TESTNUM;
                while (testnum--)
                {
                    pmulti_th->commitTask(singletest4, testnum);
                }
                } );
            //thread1��Ӷ�ȡ����
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