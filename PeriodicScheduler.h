/**
  C++ Multithreaded Periodic Task Scheduler

  PeriodicScheduler.h

  Purpose:
  Header file for Task structure and PeriodicScheduler

  @author Anish Singh Shekhawat
  @version 1.0 05/14/2017
*/
#pragma once
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <boost\thread.hpp>
#include <boost\bind.hpp>

/**
	Task Structure

	@member func Block of code that Task executes
	@member time Time when the task is to be executed
	@member interval Interval at which task is repeated
	@member name Task name
	@member uid Task ID
*/
struct Task
{
	boost::function<void()> func;
	std::chrono::system_clock::time_point time;
	std::chrono::seconds interval;
	std::string name;
	std::uint32_t uid;

	// Default task constructor
	Task()
	{}

	/**
		Task Constructor

		@param id Task ID
		@param n Task name
		@param f Task function
		@param tp Time when the task is to be executed
		@param s Task interval
	*/
	Task(const std::uint32_t id, std::string const& n, std::function<void()> f, const std::chrono::system_clock::time_point tp, const std::chrono::seconds s)
		:name(n),
		func(f),
		interval(s),
		time(tp),
		uid(id)
	{}

	// Operator to execute function
	void operator()()
	{
		func();
	}
};

// Comparator to sort the priority queue
struct TimeComparator
{
	bool operator()(const Task& lhs, const Task& rhs) const
	{
		return lhs.time > rhs.time;
	}
};

/**
	PeriodicScheduler

	@member task_queue Priority Queue to schedule tasks
	@member task_queue_changed Condition Variable to notify threads when task queue is changed
	@member queue_mutex Mutex to lock while reading or writing to task queue
	@member delete_task_set Delete task set to delete tasks
	@member update_task_set Update task set to update tasks
	@member executing Bool value to start or stop Scheduler
*/
class PeriodicScheduler
{
private:
	std::priority_queue<Task, std::deque<Task>, TimeComparator> task_queue;
	std::condition_variable task_queue_changed;
	std::mutex queue_mutex;
	std::unordered_set<std::uint32_t> delete_task_set;
	std::unordered_map<std::uint32_t, std::chrono::seconds> update_task_set;
	bool executing = true;

public:
	// PeriodicScheduler constructor 
	PeriodicScheduler();

	/**
	  Generates a unique ID for each task

	  @return A unique integer
	*/
	static int getUid();

	/**
	  Schedules a task for execution

	  @param id Task ID
	  @param n Task name
	  @param f void function that the task executes
	  @param tp Timepoint at which the task is scheduled to be executed
	  @param s Interval at which task is executed
	*/
	void schedule_periodic(const std::uint32_t &id, std::string const& n, std::function<void()> f, const std::chrono::system_clock::time_point &tp, const int &s);

	/**
	  Function that executes task in a loop
	*/
	void execute_tasks();

	/**
	  Displays a list of task currently queued
	*/
	void get_tasks_overview();

	/**
	  Converts time_point structure to time_t format for printing the time

	  @param tp Time Point to be converted into print friendly
	  @return Time point in time_t format
	*/
	std::time_t get_time_to_print(const std::chrono::system_clock::time_point &tp);

	/**
	  Adds the task ID to delete set so that the task is deleted

	  @param task_id Task ID
	*/
	void delete_task(const std::uint32_t &task_id);

	/**
	  Adds the task ID to update set to update the task

	  @param task_id Task ID
	  @param sec New interval
	*/
	void update_task(const std::uint32_t &task_id, const int &sec);

	/**
	  Runs the scheduler
	*/
	void run();

	/**
	  Stops the scheduler
	*/
	void stop();
};