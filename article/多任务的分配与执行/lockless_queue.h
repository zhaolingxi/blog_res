#pragma once
#include<functional>

using FunC_V = std::function<void()>;
using Task = FunC_V;


class lockless_queue
{
public:
	lockless_queue() {};
	virtual ~lockless_queue() {};

	virtual void push()=0;
	virtual void pop() = 0;

};

