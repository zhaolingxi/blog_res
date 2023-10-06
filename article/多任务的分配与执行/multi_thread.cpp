#include "multi_thread.h"
multi_thread* multi_thread::_pth=nullptr;
std::once_flag multi_thread::once_fg;

using namespace std;

multi_thread* multi_thread::getInstance()
{
	std::call_once(once_fg,[]() {
		_pth=new multi_thread(0);
	});
	return _pth;
}

multi_thread::multi_thread(int inum)
{
	if (inum < 0)return;
	_pth = this;
	int maxnum = (inum < MAX_THREADNUM) ? inum:MAX_THREADNUM;
	addThread(inum);
}

multi_thread::~multi_thread()
{
	_run = false;
	_cond_task.notify_all();//唤醒所有任务
	for (auto & thread : _vec_pool)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

bool multi_thread::addThread(int inum)
{
	auto add_thread = [this]{
		Task task;
		{
			unique_lock<mutex>lock{ _lock };
			auto pre_lock = [this]()->bool {return(_run || !_que_task.empty()); };
			_cond_task.wait(lock, pre_lock);//前置条件：任务队列不为空且线程池工作(此处不阻塞)
			if (!_run)return;
			if (_que_task.empty())return;
			
			task = move(_que_task.front());
			_que_task.pop();
		}
		_idle_num--;
		task();
		_idle_num++;
	};
	while (inum--)
	{
		_vec_pool.emplace_back(add_thread);
		_idle_num++;
	}

	return true;
}
