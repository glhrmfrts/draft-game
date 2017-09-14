#ifndef DRAFT_THREAD_POOL_H
#define DRAFT_THREAD_POOL_H

#include <atomic>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>

typedef void (job_func)(void *);

struct job
{
	job_func *Func;
	void     *Arg;
};

struct thread_pool;
struct thread_data
{
	std::thread Thread;
	std::queue<job> Jobs;
	std::condition_variable ConditionVar;
	std::mutex Mutex;
	thread_pool *Pool;
    int ID;
	bool Stop = false;

	thread_data() {}
	thread_data(thread_data &&rhs) {}
};

struct thread_pool
{
	std::vector<thread_data> Threads;
	std::mutex Mutex;
	int JobsPerThread = 1;
    std::atomic_int NumJobs;
};

#endif
