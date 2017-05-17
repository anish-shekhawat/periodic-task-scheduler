#include <ctime>
#include <iostream>
#include <iomanip>
#include <functional>
#include <boost\thread.hpp>
#include <boost\asio\io_service.hpp>
#include <boost\asio.hpp>
#include <boost\bind.hpp>
#include <boost\noncopyable.hpp>
#include <boost\date_time\posix_time\posix_time.hpp>

void log_text(std::string const& text)
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	Sleep(200);
	std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S") << " " << text << " : " << boost::this_thread::get_id() << std::endl;
}

class PeriodicTask : boost::noncopyable
{
public:
	typedef std::function<void()> handler_fn;

	PeriodicTask(boost::asio::io_service& ioService
		, std::string const& name
		, int interval
		, handler_fn task)
		: ioService(ioService)
		, interval(interval)
		, task(task)
		, name(name)
		, timer(ioService)
	{
		//log_text("Create PeriodicTask '" + name + "'");
		// Schedule start to be ran by the io_service
		ioService.post(boost::bind(&PeriodicTask::start, this));
	}

	void execute(boost::system::error_code const& e)
	{
		if (e != boost::asio::error::operation_aborted) {
			log_text("Execute PeriodicTask '" + name + "'");

			task();

			timer.expires_at(timer.expires_at() + boost::posix_time::seconds(interval));
			timer.async_wait(boost::bind(&PeriodicTask::execute, this, boost::asio::placeholders::error));
		}
	}

	void start()
	{
		log_text("Start PeriodicTask '" + name + "'");

		// Uncomment if you want to call the handler on startup (i.e. at time 0)
		task();

		timer.expires_from_now(boost::posix_time::seconds(interval));
		timer.async_wait(boost::bind(&PeriodicTask::execute
			, this
			, boost::asio::placeholders::error));
	}

	std::string get_task_name()
	{
		return this->name;
	}

	int get_task_interval()
	{
		return this->interval;
	}

private:
	/*void start_wait()
	{
		timer.async_wait(boost::bind(&PeriodicTask::execute
			, this));
	}*/

private:
	boost::asio::io_service& ioService;
	boost::asio::deadline_timer timer;
	handler_fn task;
	std::string name;
	int interval;
};

class PeriodicScheduler : boost::noncopyable
{
public:
	void run()
	{
		std::cout << "\n#Boost Concurrent: *" << boost::thread::hardware_concurrency() << "*#\n";
		for (unsigned t = 0; t < 3; t++)
		{
			threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
		}
	}

	void addTask(std::string const& name
		, PeriodicTask::handler_fn const& task
		, int interval)
	{
		tasks.push_back(std::make_unique<PeriodicTask>(std::ref(io_service)
			, name, interval, task));
	}

	void listTasks()
	{
		for (auto&& ptr : tasks)
		{
			std::cout << ptr->get_task_name() << std::endl;
		}
			
	}

private:
	boost::asio::io_service io_service;
	boost::thread_group threadpool;

	std::vector<std::unique_ptr<PeriodicTask>> tasks;
};

int main()
{
	PeriodicScheduler scheduler;

	scheduler.addTask("CPU", boost::bind(log_text, "* CPU USAGE"), 5);
	scheduler.addTask("Memory", boost::bind(log_text, "* MEMORY USAGE"), 10);

	log_text("Start io_service");
	scheduler.run();
	//boost::thread bt(boost::bind(&PeriodicScheduler::run, &scheduler));
	std::cout << "Task List: \n";
	scheduler.listTasks();
	return 0;
}
