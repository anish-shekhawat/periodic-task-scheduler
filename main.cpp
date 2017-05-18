#include <ctime>
#include <iostream>
#include <queue>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <boost\thread.hpp>
#include <boost\asio\io_service.hpp>
#include <boost\asio.hpp>
#include <boost\bind.hpp>
#include <boost\date_time\posix_time\posix_time.hpp>

struct Task
{
	boost::function<void()> func;
	std::chrono::system_clock::time_point time;
	std::chrono::seconds interval;
	std::string name;

	Task()
	{}

	Task(std::string const& n, std::function<void()> f, const std::chrono::system_clock::time_point tp, const std::chrono::seconds s)
		:name(n),
		 func(f),
		 interval(s),
		 time(tp)
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
	std::priority_queue<Task, std::vector<Task>, TimeComparator> task_queue;
	std::condition_variable task_queue_changed;
	std::mutex Mutex;
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

	void schedule_periodic(std::string const& n, std::function<void()> f, const std::chrono::system_clock::time_point &tp, const int &s)
	{
		Task T(n, f, tp, std::chrono::seconds(s));
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
					schedule_periodic(task.name, task.func, std::chrono::system_clock::now() + task.interval, task.interval.count());
					task.func();
					
				}
			}
			
		}
	}

	void get_tasks_overview()
	{
		std::priority_queue<Task, std::vector<Task>, TimeComparator> temp = task_queue;
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

	void run()
	{
		boost::thread_group microThreads;
		for (int i = 0; i < 2; i++)
			microThreads.create_thread(boost::bind(&PeriodicScheduler::execute_tasks, this));
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
	scheduler.schedule_periodic("Memory", boost::bind(log_text, "* Memory USAGE"), std::chrono::system_clock::now(), 10);
	scheduler.get_tasks_overview();
	scheduler.run();
	return 0;

}
