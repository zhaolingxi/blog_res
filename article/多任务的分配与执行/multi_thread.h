#pragma once
//ath:zhaolingxi<2023.10>


#include<thread>
#include<future>
#include<vector>
#include<queue>
#include<functional>
#include<atomic>
#include<mutex>
#include<memory>
#include<stdexcept>
#include<iostream>
#define MAX_THREADNUM 32 
using FunC_V = std::function<void()>;
using FunC_B= std::function<bool()>;
using Task = FunC_V;

class multi_thread final
{
public:
	static multi_thread* getInstance();
	virtual ~multi_thread();

	bool addThread(int inum);

	template <typename fuc,typename ...Arg>
	auto commitTask(fuc&& f, Arg&& ...args)->std::future<decltype(f(args...))>
	{
		if (!_run) throw std::runtime_error("");
		using RetType = decltype(f(args...));//收集返回的类型值，匹配出一个函数来
		auto task = std::make_shared<std::packaged_task<RetType()>>(//打包异步任务
			std::bind(std::forward<fuc>(f), std::forward<Arg>(args)...)
			);
		std::future <RetType>future = task->get_future();//定义异步任务返回
		{
			std::lock_guard<std::mutex>lock{ _lock };
			_que_task.emplace([task]() {(*task)(); });
		}

		_cond_task.notify_one();
		return future;
	}

	std::atomic<bool> _run{ true };
private:
	multi_thread()=delete;
	multi_thread(int inum);
	multi_thread(const multi_thread&) = delete;
	multi_thread operator()() = delete;

	static multi_thread* _pth;
	static std::once_flag once_fg;

	std::vector<std::thread> _vec_pool;
	std::queue<Task> _que_task;
	volatile std::atomic<int> _idle_num = 0;
	volatile std::atomic<int> _max_num = 0;

	std::mutex _lock;
	std::condition_variable _cond_task;
};


