#include <ctime>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
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
#include <Psapi.h>

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
	std::unordered_map<std::uint32_t, std::chrono::seconds> update_task_set;
	bool executing = TRUE;

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

	void schedule_periodic(const std::uint32_t &id, std::string const& n, std::function<void()> f, const std::chrono::system_clock::time_point &tp, const int &s)
	{
		Task T(id, n, f, tp, std::chrono::seconds(s));
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
					//std::cout << "\nExecuteA";
					while (task_queue.empty() && executing) {
						task_queue_changed.wait(lock);
					}
				}
				if (!task_queue.empty() && task_queue.top().time <= now)
				{
					
					task = task_queue.top();
					//std::cout << "\n" << task.name << " Time: #" << get_time_to_print(task.time) << "# #Now: " << get_time_to_print(now);
					task_queue.pop();
					lock.unlock();
					if (delete_task_set.find(task.uid) == delete_task_set.end())
					{	
						auto search = update_task_set.find(task.uid);
						if (search == update_task_set.end())
						{
							schedule_periodic(task.uid, task.name, task.func, std::chrono::system_clock::now() + task.interval, task.interval.count());	
						}
						else 
						{
							schedule_periodic(task.uid, task.name, task.func, std::chrono::system_clock::now() + search->second, search->second.count());
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
		std::unique_lock<std::mutex> lock(Mutex);
		std::priority_queue<Task, std::deque<Task>, TimeComparator> temp = task_queue;
		lock.unlock();
		std::cout << "+------" << "+---------------------" << "+-------------+" << std::endl;
		std::cout << "|" << std::setw(5) << "UID" << " |" << std::setw(20) << "Task Name" << " |" << std::setw(12) << "Interval" << " |" << std::endl;
		std::cout << "+------" << "+---------------------" << "+-------------+" << std::endl;
		while (!temp.empty()) {
			std::cout << "|" << std::setw(5) << temp.top().uid << " |" << std::setw(20) << temp.top().name << " |" << std::setw(12) << temp.top().interval.count() << " |" << std::endl;
			std::cout << "+------" << "+---------------------" << "+-------------+" << std::endl;
			//std::cout << temp.top().name << " : " << temp.top().uid << " : " << temp.top().interval.count() << " : " << get_time_to_print(temp.top().time) << std::endl;
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
		//std::cout << "Delete Called!";
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

	void stop()
	{
		executing = FALSE;
	}
};

void physical_memory_usage()
{
	PROCESS_MEMORY_COUNTERS_EX pmc_ex;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc_ex, sizeof(pmc_ex));
	std::size_t physical_mem_usage = pmc_ex.PrivateUsage;
	printf("%zu\n", physical_memory_usage);
}

void virtual_memory_usage()
{
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	DWORDLONG totalVirtualMem = memInfo.ullTotalPageFile;
	DWORDLONG virtualMemUsed = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;
	std::cout << totalVirtualMem << std::endl;
}

int main()
{
	PeriodicScheduler scheduler;
	scheduler.schedule_periodic(scheduler.getUid(), "VIRTUAL MEM USAGE", boost::bind(virtual_memory_usage), std::chrono::system_clock::now(), 5);
	scheduler.schedule_periodic(scheduler.getUid(), "PHY MEM USAGE", boost::bind(physical_memory_usage), std::chrono::system_clock::now(), 10);
	
	boost::thread th(&PeriodicScheduler::run, &scheduler);
	
	std::uint32_t taskid;
	int interval = 5;
	// user's entered option will be saved in this variable
	int option; 
	//do-while loop starts here.that display menu again and again until user select to exit program
	do
	{
		//Displaying Options for the menu
		std::cout << "1) Display Task List " << std::endl;
		std::cout << "2) Add new Task " << std::endl;
		std::cout << "3) Update Task Interval " << std::endl;
		std::cout << "4) Delete Task" << std::endl;
		std::cout << "5) Exit Program " << std::endl;
		//Prompting user to enter an option according to menu
		std::cout << "Please select an option : ";
		std::cin >> option;  

		switch (option)
		{
		case 1:
			scheduler.get_tasks_overview();
			break;

		case 2:
			int task_option;
			std::cout << "Enter Task interval ";
			std::cin >> interval;
			std::cout << "Enter Task type to schedule: " << std::endl;
			std::cout << "1) Physical Memory Usage " << std::endl;
			std::cout << "2) Virtual Memory Usage " << std::endl;
			std::cout << "Please select an option : ";
			std::cin >> task_option;
			if (task_option == 1)
			{
				try {
					scheduler.schedule_periodic(scheduler.getUid(),
						"CPU2",
						boost::bind(physical_memory_usage),
						std::chrono::system_clock::now(), interval);
				}
				catch (std::exception const &e){
					std::cout << e.what() << std::endl;
				}
			}
			else if (task_option == 2)
			{
				try {
					scheduler.schedule_periodic(scheduler.getUid(),
						"Memory2",
						boost::bind(virtual_memory_usage),
						std::chrono::system_clock::now(), interval);
				}
				catch (std::exception const &e)
				{
					std::cout << e.what() << std::endl;
				}
				
			}
			else
			{
				std::cout << "Wrong Input!";
			}
			break;

		case 3:
			scheduler.get_tasks_overview();
			std::cout << "Enter Task UID and new Interval separated by spaces to update ";
			std::cin >> taskid >> interval;
			scheduler.update_task(taskid, interval);
			break;

		case 4:
			std::uint32_t taskid;
			scheduler.get_tasks_overview();
			std::cout << "Enter Task UID to delete ";
			std::cin >> taskid;
			scheduler.delete_task(taskid);
			break;

		case 5:
			scheduler.stop();
			break;

		default:
			std::cout << "Incorrect option entered!" << std::endl;
		}
	}
	while (option != 5);
	return 0;
}
