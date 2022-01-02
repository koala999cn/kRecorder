#pragma once


// 多线程的使用函数

class KuThread
{
public:
	static void wait(float time_in_second);

private:
	KuThread();
	~KuThread();
};