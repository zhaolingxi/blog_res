<文章更新中>
## 前言
并发一直以来都是学习编程从入门到初级的一个必经之路，毕竟一个单线程的程序是无法适应现在的用户高要求的。
多进程、多线程是提升一款产品生产效率的最简单、也是最常见的方式。
一个程序往往以一个进程为单位，进程是资源划分的整体。
而一个程序的执行，往往是线程为基本单位。
我们这里将使用最简单的方式来进行C++线程池的实现。

## 线程池
首先，我们来模拟手撕一个线程池。
明确需求，使用C++,有一定的跨平台的移植能力。高效，易读。
我们这里选择了C++自带的thread库，它不受操作系统限制，随时编译随时使用。
同时也是足够的高效与简洁，有人免费维护升级。

那首先我们来设计一下一个线程池需要的基本参数和接口。
参数部分：
线程池承载的线程数量
线程池的工作状态
是否支持自增长

依据参数需求，
我们需要一个计数_idlThrNum，记录当前线程个数；
一个池子，存放线程资源
一个模板化的任务函数function<void()>;
一个存储任务的队列queue<Task>;
一把锁mutex
一个用来唤醒线程的condition_variable

接口部分：
新建线程池接口
删除线程池接口
添加线程任务接口

依据接口需求，我们拟定需要一个构造函数threadpool，一个析构函数~threadpool
一个addThread添加线程进入池子；
一个commit提交线程任务

OK，那我们就开始构建头文件：
```
#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#define _CRT_SECURE_NO_WARNINGS
#include<vector>
#include<queue>
#include<atomic>
#include<future>
#include<stdexcept>

namespace std
{
#define THREAD_MAX_NUM 16

	#define threadpool_API __declspec(dllexport)//动态链接库
	class threadpool_API threadpool
	{
	public:
		using Task = function<void()>;
		vector<thread> _pool;
		queue<Task> _tasks;
		mutex _lock;
		condition_variable _task_cv;
		atomic<bool> _run{ true };
		atomic<int> _idlThrNum = 0;//声明原子操作，避免出现计数错误
	public:
		inline threadpool(unsigned short size = 4) { addThread(size); }
		inline~threadpool()
		{
			_run = false;
			_task_cv.notify_all();
			for (thread& thread : _pool)
			{
				if (thread.joinable())
				{
					thread.join();
				}
			}
		}

	public:
		template<class F, class ...Args>
		auto commit(F&& f, Args &&...args)->future<decltype(f(args...))>
		{
			
		}

		int idlCount() { return _idlThrNum; }
		int thrCount() { return _pool.size(); }
#ifndef THREAD_AUTO_GROW
	private:
#endif
		void addThread(unsigned short size);		
#endif // THREAD_POOL_H
	};
	
}