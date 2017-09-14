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
			Data->ConditionVar.wait(Lock, [Data]{ return !Data->Jobs.empty() || Data->Stop; });
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

void CreateThreadPool(thread_pool &Pool, int NumThreads, int JobsPerThread = 1)
{
	Pool.JobsPerThread = JobsPerThread;
	Pool.Threads.resize(NumThreads);
	for (int i = 0; i < NumThreads; i++)
	{
		auto &Data = Pool.Threads[i];
        Data.ID = i;
		Data.Pool = &Pool;
		Data.Thread = std::thread(ThreadPoolLoop, &Data);
	}
}

void AddJob(thread_pool &Pool, job_func *Func, void *Arg)
{
	for (auto &Data : Pool.Threads)
	{
		std::unique_lock<std::mutex> Lock(Data.Mutex, std::defer_lock);
		Lock.lock();
		if (Data.Jobs.size() < Pool.JobsPerThread)
		{
			Data.Jobs.push(job{ Func, Arg });
			Pool.NumJobs++;

			Lock.unlock();
			Data.ConditionVar.notify_one();
			break;
		}
		else
		{
			Lock.unlock();
		}
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
