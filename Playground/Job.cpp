#include "Job.h"
#include "Main.h"

thread_local static Job g_jobAllocator[MAX_JOB_COUNT];
thread_local static u32 g_allocatedJobs;

Job* AllocateJob()
{
	const u32 index = g_allocatedJobs++;
	Job* allocatedJob = &g_jobAllocator[(index - 1u) & (MAX_JOB_COUNT - 1u)]; //ring buffer
	memset(allocatedJob, 0x00, sizeof(Job));
	return allocatedJob;
}

Job* CreateJob(JobFunction function)
{
	Job* job = AllocateJob();
	job->function = function;
	job->parent = nullptr;
	job->unfinishedJobs = 1;

	return job;
}

Job* CreateJobAsChild(Job* parent, JobFunction function)
{
	parent->unfinishedJobs++; //atomically incremented

	Job* job = AllocateJob();
	job->function = function;
	job->parent = parent;
	job->unfinishedJobs = 1;

	return job;
}

void Finish(Job* job)
{
	job->unfinishedJobs--; //atomically decrement
	const size_t unfinishedJobs = job->unfinishedJobs;
	if (unfinishedJobs == 0 && (job->parent))
	{
		Finish(job->parent);
	}
}

void Execute(Job* job)
{
	(job->function)(job, job->data);
	Finish(job);
}

bool IsEmptyJob(const Job* job)
{
	return job == nullptr;
}

bool HasJobCompleted(const Job* job)
{
	return job->unfinishedJobs == 0;
}