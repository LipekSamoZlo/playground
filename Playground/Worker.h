#pragma once

#include <string>
#include "Common.h"
#include "Job.h"

struct Job;

enum EWorkerAffinity
{
	MAIN,
	WORKER,
};

enum EWorkerStatus
{
	CREATED,
	RUNNING,
	SUSPENDED,
	STOPPED,
	DESTROYED,
};

#define LOCKLESS 0

struct Worker
{
	Worker(const std::string& _name, EWorkerAffinity _affinity, bool _suspended);
	~Worker();

	std::string name;
	EWorkerAffinity affinity;
	EWorkerStatus workerStatus;

	std::thread::id threadId;
	std::thread* thread;

	std::atomic_size_t jobsDone;

	struct WorkStealingQueue* queue;

	void Start();
	void Stop();

	void DoJob();
};

static const u32 MAX_WORKER_COUNT = 4;
const u32 g_workerThreadCount = MAX_WORKER_COUNT;

void CreateWorker(Worker* w);
Worker* GetWorker(int i);

void Run(Job* job);
void Wait(const Job* job);
Job* GetJob(void);

struct WorkStealingQueue
{
	WorkStealingQueue();

	Job* jobs[MAX_JOB_COUNT];

	std::atomic<s32> bottom;
	std::atomic<s32> top;

	void Push(Job* job);
	Job* Pop();
	Job* Steal();

	void PushLock(Job* job);
	Job* PopLock();
	Job* StealLock();
};

WorkStealingQueue* GetWorkerThreadQueue();