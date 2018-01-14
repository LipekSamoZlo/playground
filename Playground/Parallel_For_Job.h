#pragma once

#include "Common.h"
#include "Job.h"

class CountSplitter
{
public:
	explicit CountSplitter(unsigned int count)
		: m_count(count)
	{
	}

	template <typename T>
	inline bool Split(unsigned int count) const
	{
		return (count > m_count);
	}

private:
	unsigned int m_count;
};

class DataSizeSplitter
{
public:
	explicit DataSizeSplitter(unsigned int size)
		: m_size(size)
	{
	}

	template <typename T>
	inline bool Split(unsigned int count) const
	{
		return (count * sizeof(T) > m_size);
	}

private:
	unsigned int m_size;
};

template <typename T, typename S>
Job* Parallel_For(T* data, u32 count, void(*function)(T*, u32), const S& splitter)
{
	typedef parallel_for_job_data<T, S> JobData;
	const JobData jobData(data, count, function, splitter);
	
	Job* job = CreateJob(&parallel_for_job<JobData>, jobData);

	return job;
}

template <typename T, typename S>
struct parallel_for_job_data
{
	typedef T DataType;
	typedef S SplitterType;

	parallel_for_job_data(DataType* _data, u32 _count, void(*_function)(DataType*, u32), const SplitterType* _splitter)
		: data(_data)
		, count(_count)
		, function(_function)
		, splitter(_splitter)
	{
	}

	DataType* data;
	u32 count;
	void(*function)(DataType*, u32);
	SplitterType splitter;
};

template <typename JobData>
void parallel_for_job(Job* job, const void* jobData)
{
	const JobData* data = static_cast<const jobData*>(jobData);
	const JobData::SplitterType& splitter = data->splitter;

	if (splitter.Split<JobData::DataType>(data->count))
	{
		//split in two
		const u32 leftCount = data->count / 2u;
		const JobData leftData(data->data, leftCount, data->function, splitter);
		Job* left = CreateJobAsChild(job, &jobs::parallel_for_job<JobData>, leftData);
		Run(left);

		const u32 rightCount = data->count - leftCount;
		const JobData rightData(data->data + leftCount, rightCount, data->function, splitter);
		Job* right = CreateJobAsChild(job, &jobs::parallel_for_job<JobData>, rightData);
		Run(right);
	}
	else
	{
		//execute the function on the range of data
		(data->function)(data->data, data->count);
	}
}