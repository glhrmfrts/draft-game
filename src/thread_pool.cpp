// Copyright

static void ThreadPoolLoop(void *Arg)
{
    auto *Data = (thread_data *)Arg;
    auto *Pool = Data->Pool;
    for (;;)
    {
		job Job;
		{
			std::unique_lock<std::mutex> Lock(Data->Mutex);
			Data->ConditionVar.wait(Lock, [Data] { return !Data->Jobs.empty() || Data->Stop; });
			if (Data->Stop)
			{
				return;
			}

			Job = Data->Jobs.front();
			Data->Jobs.pop();
		}

		Job.Func(Job.Arg);
		Pool->NumJobs--;
    }
}

void CreateThreadPool(thread_pool &pool, int MaxThreads, int JobsPerThread = 1)
{
    pool.NumJobs = 0;
    pool.JobsPerThread = JobsPerThread;
	pool.MaxThreads = MaxThreads;
	pool.NumThreads = 0;
	pool.Threads.resize(MaxThreads);
}

void AddJob(thread_pool &pool, job_func *func, void *arg)
{
	bool grow = true;
	for (int i = 0; i < pool.NumThreads; i++)
	{
		auto data = &pool.Threads[i];
		std::unique_lock<std::mutex> Lock(data->Mutex, std::defer_lock);
		Lock.lock();
		if ((int)data->Jobs.size() < pool.JobsPerThread)
		{
			data->Jobs.push(job{ func, arg });
			pool.NumJobs++;

			Lock.unlock();
			data->ConditionVar.notify_one();
			grow = false;
			break;
		}
		else
		{
			Lock.unlock();
		}
	}

	if (grow && pool.NumThreads < pool.MaxThreads)
	{
		pool.NumThreads++;
		pool.NumJobs++;

		auto data = &pool.Threads[pool.NumThreads - 1];
		data->Jobs.push(job{ func, arg });
		data->ID = pool.Threads.size() - 1;
		data->Pool = &pool;
		data->Thread = std::thread(ThreadPoolLoop, data);
	}
}

void DestroyThreadPool(thread_pool &Pool)
{
    assert(Pool.NumJobs == 0);
    for (auto &Data : Pool.Threads)
    {
        std::unique_lock<std::mutex> Lock(Data.Mutex);
        Data.Stop = true;
        Data.ConditionVar.notify_one();
    }

    for (auto &Data : Pool.Threads)
    {
        Data.Thread.join();
    }
}
