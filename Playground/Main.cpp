
#include <string>
#include <iostream>
#include <chrono>

#include "Main.h"
#include "Job.h"
#include "Parallel_For_Job.h"
#include "Worker.h"

#define TEST 1

void empty_job(Job* j, const void*)
{
}

int main()
{
	for (int i = 0; i < MAX_WORKER_COUNT; i++) //3 workers for 3 cores + 1 for main core
	{
		Worker* w = nullptr;

		if (i != MAX_WORKER_COUNT - 1)
		{
			w = new Worker(std::string("Worker_") + std::to_string(i), WORKER, true);
		}
		else
		{
			w = new Worker(std::string("Worker_MAIN"), MAIN, true);
		}

		CreateWorker(w);
	}

	const std::size_t N = MAX_JOB_COUNT - 1;

	for (int i = 0; i < MAX_WORKER_COUNT; i++)
	{
		GetWorker(i)->Start();
	}

#if TEST == 0
	for (unsigned int i = 0; i < N; ++i)
	{
		Job* job = CreateJob(&empty_job);
		Run(job);
		Wait(job);
	}

#elif TEST == 1
	PROFILE_START;

	Job* root = CreateJob(&empty_job);
	for (unsigned int i = 0; i < N; ++i)
	{
		Job* job = CreateJobAsChild(root, &empty_job);
		Run(job);
	}

	Run(root);
	Wait(root);

	PROFILE_END;
#elif TEST == 2

	Job* job = Parallel_For(g_particles, 100000, &UpdateParticles, DataSizeSplitter(32 * 1024));
	Run(job);
	Wait(job);

#endif

	std::size_t workersJobDone = 0;
	for (int i = 0; i < MAX_WORKER_COUNT - 1; i++)
	{
		Worker* w = GetWorker(i);
		std::cout << w->name << " jobs done: " << std::to_string(w->jobsDone) << std::endl;
		workersJobDone += w->jobsDone;
	}

	Worker* w = GetWorker(MAX_WORKER_COUNT - 1);
	std::cout << w->name << " jobs done: " << std::to_string(N - workersJobDone) << std::endl;

	__debugbreak(); //all jobs are done

	for (int i = 0; i < MAX_WORKER_COUNT; i++)
	{
		Worker* w = GetWorker(i);
		w->Stop();

		delete w;
	}
	return 0;
}