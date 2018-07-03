#pragma once

#include "Common.h"

struct Job;
typedef void(*JobFunction)(Job*, const void*);

struct Job
{
	Job()
		: function(nullptr)
		, parent(nullptr)
		, unfinishedJobs(-1)
	{
		//memset(data, 0x00, 64u - sizeof(JobFunction) + sizeof(Job*) + sizeof(as32));
	}

	JobFunction function;
	Job* parent;
	std::atomic_size_t unfinishedJobs; //atomic - job is completed when this == -1

	static constexpr std::size_t JOB_PAYLOAD_SIZE = sizeof(JobFunction) + sizeof(Job*) + sizeof(std::atomic_size_t);
	static constexpr std::size_t JOB_MAX_PADDING_SIZE = L1_CACHE_LINE_SIZE;
	static constexpr std::size_t JOB_PADDING_SIZE = JOB_MAX_PADDING_SIZE - JOB_PAYLOAD_SIZE;

	char data[JOB_PADDING_SIZE]; //padding
};

const u32 MAX_JOB_COUNT = 1 << 16; //1 << 12;
static const u32 MASK = MAX_JOB_COUNT - 1u;

bool IsEmptyJob(const Job* job);
bool HasJobCompleted(const Job* job);

Job* AllocateJob();

Job* CreateJob(JobFunction function);
Job* CreateJobAsChild(Job* parent, JobFunction function);
void Finish(Job* job);
void Execute(Job* job);