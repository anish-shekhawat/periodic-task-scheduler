#include <ctime>
#include <iostream>
#include <Windows.h>
#include <queue>
#include <chrono>
#include <cstdlib>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <boost/thread.hpp>
#include <boost\thread.hpp>
#include <boost\bind.hpp>

struct Task
{
	boost::function<void()> func;
	std::chrono::system_clock::time_point time;
	std::chrono::seconds interval;
	std::string name;
	std::uint32_t uid;

	Task()
	{}

	Task(const std::uint32_t id, std::string const& n, std::function<void()> f, const std::chrono::system_clock::time_point tp, const std::chrono::seconds s)
		:name(n),
		 func(f),
		 interval(s),
		 time(tp),
		 uid(id)
	{}

	void operator()()
	{
		func();
	}
};

struct TimeComparator
{
	bool operator()(const Task& lhs, const Task& rhs) const
	{
		return lhs.time > rhs.time;
	}
};

class PeriodicScheduler
{
private:
	int num_threads;
	std::priority_queue<Task, std::deque<Task>, TimeComparator> task_queue;
	std::condition_variable task_queue_changed;
	std::mutex Mutex;
	std::unordered_set<std::uint32_t> delete_task_set;
	std::unordered_map<std::uint32_t, std::chrono::seconds> update_tas
	bool executing=true;

public:
	PeriodicScheduler():num_threads(0)
	{}
	
	void schedule(Task T)
	{
		{
			std::unique_lock<std::mutex> lock(Mutex);
			task_queue.push(T);
		}
		task_queue_changed.notify_one();
	}

	static int getUid()
	{
		static std::atomic<std::uint32_t> uid{ 0 };
		return ++uid;
	}

	void schedule_periodic(std::string const& n, std::function<void()> f, const std::chrono::system_clock::time_point &tp, const int &s)
	{
		Task T(getUid(), n, f, tp, std::chrono::seconds(s));
		schedule(T);
	}

	void execute_tasks()
	{
		while (executing) 
		{
			Task task;
			auto now = std::chrono::system_clock::now();
			{
				std::unique_lock<std::mutex> lock(Mutex);
				if (task_queue.empty() && executing)
				{
					std::cout << "\nExecuteA";
					while (task_queue.empty() && executing) {
						task_queue_changed.wait(lock);
					}
				}
				if (!task_queue.empty() && task_queue.top().time <= now)
				{
					
					task = task_queue.top();
					std::cout << "\nTime: #" << get_time_to_print(task.time) << "# #Now: " << get_time_to_print(now);
					task_queue.pop();
					lock.unlock();
					if (delete_task_set.find(task.uid) == delete_task_set.end())
					{	
						auto search = update_task_set.find(task.uid);
						if (search == update_task_set.end())
						{
							schedule_periodic(task.name, task.func, std::chrono::system_clock::now() + task.interval, task.interval.count());	
						}
						else 
						{
							schedule_periodic(task.name, task.func, std::chrono::system_clock::now() + search->second, search->second.count());
							std::unique_lock<std::mutex> lock(Mutex);
							update_task_set.erase(search);

						}
						task.func();
					}
					else
					{
						std::unique_lock<std::mutex> lock(Mutex);
						delete_task_set.erase(task.uid);
					}
					
				}
			}
			
		}
		return;
	}

	void get_tasks_overview()
	{
		std::priority_queue<Task, std::deque<Task>, TimeComparator> temp = task_queue;
		while (!temp.empty()) {
				
			std::cout << temp.top().name << " : " << temp.top().interval.count() << " : " << get_time_to_print(temp.top().time) << std::endl;
			temp.pop();
		}

	}

	std::time_t get_time_to_print(const std::chrono::system_clock::time_point &tp)
	{
		auto time_point = tp;
		std::time_t time_c = std::chrono::system_clock::to_time_t(time_point);
		return time_c;
	}

	void delete_task(const std::uint32_t &task_id)
	{
		delete_task_set.insert(task_id);
	}

	void update_task(const std::uint32_t &task_id, const int &sec)
	{
		update_task_set[task_id] = std::chrono::seconds(sec);
	}

	void run()
	{
		boost::thread_group microThreads;
		for (int i = 0; i < 2; i++)
			microThreads.create_thread(boost::bind(&PeriodicScheduler::execute_tasks, this));

		microThreads.join_all();
	}
};

void log_text(std::string const& text)
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S") << " " << text << " : " << boost::this_thread::get_id() << std::endl;
}

int main()
{
	PeriodicScheduler scheduler;
	scheduler.schedule_periodic("CPU", boost::bind(log_text, "* CPU USAGE"), std::chrono::system_clock::now(), 5);
	scheduler.get_tasks_overview();
	boost::thread th(&PeriodicScheduler::run, &scheduler);
	Sleep(600);
	scheduler.schedule_periodic("Memory", boost::bind(log_text, "* Memory USAGE"), std::chrono::system_clock::now(), 10);
	th.join();
	return 0;
}
