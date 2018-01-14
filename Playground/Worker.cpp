#include "Worker.h"
#include "Job.h"
#include "Main.h"

static Worker* g_workers[MAX_WORKER_COUNT];
static u16 g_workersCount = 0;

void workerMain(const void* data)
{
	((Worker*)data)->DoJob();
}

Worker::Worker(const std::string& _name, EWorkerAffinity _affinity, bool _suspended)
{
	name = _name;
	affinity = _affinity;
	workerStatus = _suspended ? SUSPENDED : CREATED;

	queue = new WorkStealingQueue();
	memset(queue, 0x00, sizeof(WorkStealingQueue));
}

Worker::~Worker()
{
	if (thread != nullptr && thread->joinable())
		thread->join();

	delete thread;

	delete queue;
}

void Worker::Start()
{
	if (affinity == EWorkerAffinity::WORKER)
	{
		std::thread* worker = new std::thread(workerMain, this);
		thread = worker;
		threadId = worker->get_id();
	}
	else
	{
		thread = nullptr;
		threadId = std::this_thread::get_id();
	}

	jobsDone = 0;

	COMPILER_BARRIER;
	
	workerStatus = RUNNING;
}

void Worker::Stop()
{
	workerStatus = STOPPED;
}

void Worker::DoJob()
{
	while (workerStatus < STOPPED)
	{
		if (workerStatus == RUNNING)
		{
			Job* job = GetJob();
			if (job)
			{
				Execute(job);
				jobsDone++; //atomic
			}
		}
		
		std::this_thread::yield();
	}
}

WorkStealingQueue::WorkStealingQueue()
	: bottom(0)
	, top(0)
{
	memset(jobs, 0x00, sizeof(Job*)*MAX_JOB_COUNT);
}

void WorkStealingQueue::Push(Job* job)
{
	s32 b = bottom;
	jobs[b & MASK] = job;

	// ensure the job is written before b+1 is published to other threads.
	// on x86/64, a compiler barrier is enough.
	COMPILER_BARRIER;

	bottom.store(b + 1, std::memory_order_release);
}

Job* WorkStealingQueue::Pop()
{
	s32 b = max(0, bottom - 1);

	//std::atomic_exchange(&bottom, b);
	bottom.store(b, std::memory_order_release);

	s32 t = top;
	if (t <= b)
	{
		Job* job = jobs[b & MASK];
		if (t != b)
		{
			// there's still more than one item left in the queue
			return job;
		}

		// this is the last item in the queue
		s32 expected = t;
		s32 desired = t + 1;
		if (top.compare_exchange_weak(expected, desired, std::memory_order_acq_rel) == false)
		{
			// failed race against steal operation
			job = nullptr;
		}

		bottom = t + 1;
		return job;
	}
	else
	{
		// deque was already empty
		bottom = t;
		return nullptr;
	}
}

Job* WorkStealingQueue::Steal()
{
	s32 t = top;

	// ensure that top is always read before bottom.
	// loads will not be reordered with other loads on x86, so a compiler barrier is enough.
	COMPILER_BARRIER;

	s32 b = bottom;

	if (t < b)
	{
		Job* job = jobs[t & MASK];
		s32 expected = t;
		s32 desired = t + 1;
		if (top.compare_exchange_weak(expected, desired, std::memory_order_acq_rel) == false)
		{
			return nullptr;
		}

		return job;

	}
	else
	{
		//empty queue
		return nullptr;
	}
}

std::mutex critSec;

void WorkStealingQueue::PushLock(Job* job)
{
	std::lock_guard<std::mutex> lock(critSec);

	jobs[bottom & MASK] = job;
	++bottom;
}

Job* WorkStealingQueue::PopLock(void)
{
	std::lock_guard<std::mutex> lock(critSec);

	const int jobCount = bottom - top;
	if (jobCount <= 0)
	{
		// no job left in the queue
		return nullptr;
	}

	--bottom;
	return jobs[bottom & MASK];
}

Job* WorkStealingQueue::StealLock(void)
{
	std::lock_guard<std::mutex> lock(critSec);

	const int jobCount = bottom - top;
	if (jobCount <= 0)
	{
		// no job there to steal
		return nullptr;
	}

	Job* job = jobs[top & MASK];
	++top;
	return job;
}

void Run(Job* job)
{
	WorkStealingQueue* queue = GetWorkerThreadQueue();
	queue->Push(job);
}

void Wait(const Job* job)
{
	// wait until the job has completed. in the meantime, work on any other job.
	while (!HasJobCompleted(job))
	{
		Job* nextJob = GetJob();
		if (nextJob)
		{
			Execute(nextJob);
		}
	}
}

Job* GetJob(void)
{
	WorkStealingQueue* queue = GetWorkerThreadQueue();

	Job* job = queue->Pop();
	if (IsEmptyJob(job))
	{
		// this is not a valid job because our own queue is empty, so try stealing from some other queue
		u32 randInt = urandom(0, g_workerThreadCount - 1);

		WorkStealingQueue* stealQueue = g_workers[randInt]->queue;
		if (stealQueue == queue)
		{
			//don't try to steal from ourselves
			std::this_thread::yield();
			return nullptr;
		}

		Job* stolenJob = stealQueue->Steal();
		if (IsEmptyJob(stolenJob))
		{
			// we couldn't steal a job from the other queue either, so we just yield our time slice for now
			std::this_thread::yield();
			return nullptr;
		}

		return stolenJob;
	}

	return job;
}

WorkStealingQueue* GetWorkerThreadQueue()
{
	std::thread::id thisId = std::this_thread::get_id();

	for (int i = 0; i < MAX_WORKER_COUNT; i++)
	{
		Worker* w = g_workers[i];
		if (w->threadId == thisId)
			return g_workers[i]->queue;
	}

	return nullptr;
}

void CreateWorker(Worker* w)
{
	g_workers[g_workersCount++] = w;
}
Worker* GetWorker(int i)
{
	return g_workers[i];
}