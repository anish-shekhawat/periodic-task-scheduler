/**
  C++ Multithreaded Periodic Task Scheduler

  PeriodicScheduler.cpp

  Purpose:
  Member function implementations of PeriodicScheduler

  @author Anish Singh Shekhawat
  @version 1.0 05/14/2017
*/
#include "PeriodicScheduler.h"
#include <ctime>
#include <stdlib.h>
#include <string>
#include <iomanip>
#include <atomic>
#include <chrono>

// PeriodicScheduler default constructor
PeriodicScheduler::PeriodicScheduler()
{}

// Generate a unique id for each task
int PeriodicScheduler::getUid()
{
	static std::atomic<std::uint32_t> uid{ 0 };
	return ++uid;
}

// Schedules task in the priority queue based on the start time
void PeriodicScheduler::schedule_periodic(const std::uint32_t &id, std::string const& n, std::function<void()> f, const std::chrono::system_clock::time_point &tp, const int &s)
{
	// Create Task object
	Task T(id, n, f, tp, std::chrono::seconds(s));
	{
		// Acquire lock to push task to priority queue
		std::unique_lock<std::mutex> lock(queue_mutex);
		task_queue.push(T);
	}
	// Notify a waiting thread that a task has been pushed to the queue
	task_queue_changed.notify_one();
}

// Function that starts executing tasks
void PeriodicScheduler::execute_tasks()
{
	while (executing)
	{
		Task task;
		auto now = std::chrono::system_clock::now();
		{
			// Acquire lock to check the task queue for any task
			std::unique_lock<std::mutex> lock(queue_mutex);
			if (task_queue.empty() && executing)
			{
				// If no task in queue then wait for a task to be posted to the queue
				while (task_queue.empty() && executing) {

					//Wait untill task queue is changed and thread is notified
					task_queue_changed.wait(lock);
				}
			}
			// If task queue is not empty and the its time to execute the task
			if (!task_queue.empty() && task_queue.top().time <= now)
			{

				task = task_queue.top();
				task_queue.pop();

				// Unlocks so that task can be executed
				lock.unlock();

				// Check if task is supposed to be deleted
				if (delete_task_set.find(task.uid) == delete_task_set.end())
				{
					// Check if task is supposed to be updated
					auto search = update_task_set.find(task.uid);

					// Schedule task with the same interval
					if (search == update_task_set.end())
					{
						schedule_periodic(task.uid, task.name, task.func, std::chrono::system_clock::now() + task.interval, task.interval.count());
					}
					else
					{
						// Schedule task with the new interval
						schedule_periodic(task.uid, task.name, task.func, std::chrono::system_clock::now() + search->second, search->second.count());
						std::unique_lock<std::mutex> lock(queue_mutex);

						// Remove from to be update list once updated
						update_task_set.erase(search);

					}
					// Execute the task
					task.func();
				}
				else
				{	
					// Neither executes nor schedules the deleted task further
					std::unique_lock<std::mutex> lock(queue_mutex);

					// Remove task from to be deleted list once deleted
					delete_task_set.erase(task.uid);
				}
			}
		}
	}
	return;
}

// Display a list of task currently queued
void PeriodicScheduler::get_tasks_overview()
{
	// Acquire a lock to duplicate the queue since priority queues can't be traversed
	std::unique_lock<std::mutex> lock(queue_mutex);
	std::priority_queue<Task, std::deque<Task>, TimeComparator> temp = task_queue;

	// Release lock after duplicating
	lock.unlock();

	//Display Task List
	std::cout << "+------" << "+---------------------" << "+-------------+" << std::endl;
	std::cout << "|" << std::setw(5) << "UID" << " |" << std::setw(20) << "Task Name" << " |" << std::setw(12) << "Interval" << " |" << std::endl;
	std::cout << "+------" << "+---------------------" << "+-------------+" << std::endl;
	while (!temp.empty()) {
		std::cout << "|" << std::setw(5) << temp.top().uid << " |" << std::setw(20) << temp.top().name << " |" << std::setw(12) << temp.top().interval.count() << " |" << std::endl;
		std::cout << "+------" << "+---------------------" << "+-------------+" << std::endl;
		temp.pop();
	}

}

// Converts time_point structure to time_t format for printing the time
std::time_t PeriodicScheduler::get_time_to_print(const std::chrono::system_clock::time_point &tp)
{
	auto time_point = tp;
	std::time_t time_c = std::chrono::system_clock::to_time_t(time_point);
	return time_c;
}

// Adds the task ID to delete set so that the task is deleted
void PeriodicScheduler::delete_task(const std::uint32_t &task_id)
{
	delete_task_set.insert(task_id);
}

// Adds the task ID to update set to update the tasks
void PeriodicScheduler::update_task(const std::uint32_t &task_id, const int &sec)
{
	update_task_set[task_id] = std::chrono::seconds(sec);
}

// Runs the scheduler
void PeriodicScheduler::run()
{
	// Creates a Boost thread group
	boost::thread_group microThreads;

	// Create a pool of threads
	for (int i = 0; i < 15; i++)
		microThreads.create_thread(boost::bind(&PeriodicScheduler::execute_tasks, this));

	// Joins all thread at the end
	microThreads.join_all();
}

// Stops the scheduler
void PeriodicScheduler::stop()
{
	executing = false;
}