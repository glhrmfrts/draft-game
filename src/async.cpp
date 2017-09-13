// Copyright

typedef std::function<void(void *)> job_func;

struct job
{
    job_func Func;
    void     *Arg;
};

struct thread_pool
{
    std::vector<std::thread> Threads;
    std::queue<job> Jobs;
    std::condition_variable ConditionVar;
    std::mutex JobsMutex;
};

static void ThreadPoolLoop(void *Arg)
{
    auto *Pool = (thread_pool *)Arg;
    while (true)
    {
        job Job;
        {
            std::unique_lock<std::mutex> Lock(Pool->JobsMutex);
            Pool->ConditionVar.wait(Lock, []{return !Pool->Jobs.empty();});

            Job = Pool->Jobs.front();
            Pool->Queue.pop_front();
        }

        Job.Func(Job.Arg);
    }
}

void CreateThreadPool(thread_pool &Pool, int NumThreads)
{
    for (int i = 0; i < NumThreads; i++)
    {
        Pool.Threads.push_back(std::thread(ThreadPoolLoop, &Pool));
    }
}

void AddJob(thread_pool &Pool, job_func Func, void *Arg)
{
    {
        std::unique_lock<std::mutex> Lock(Pool.JobsMutex);
        Pool.Jobs.push(job{Func, Arg});
    }

    Pool.ConditionVar.notify_one();
}

void WaitForAllJobs(thread_pool &Pool)
{

}
