#include "KuThread.h"
#include <thread>


void KuThread::wait(float second)
{
	std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(second*1000));
}