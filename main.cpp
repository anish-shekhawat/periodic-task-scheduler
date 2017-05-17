#include <ctime>
#include <iostream>
#include <iomanip>
#include <functional>
#include <boost\thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

void log_text(std::string const& text)
{	
	std::cout << text << ": " << boost::this_thread::get_id() << "\n";
	Sleep(100);
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S") << " " << text << std::endl;
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
		log_text("Create PeriodicTask '" + name + "'");
		// Schedule start to be ran by the io_service
		ioService.post(boost::bind(&PeriodicTask::start, this));
	}

	void execute(boost::system::error_code const& e)
	{
		if (e != boost::asio::error::operation_aborted) {
			log_text("Execute PeriodicTask '" + name + "'");

			task();

			timer.expires_at(timer.expires_at() + boost::posix_time::seconds(interval));
			start_wait();
		}
	}

	void start()
	{
		log_text("Start PeriodicTask '" + name + "'");

		// Uncomment if you want to call the handler on startup (i.e. at time 0)
		// task();

		timer.expires_from_now(boost::posix_time::seconds(interval));
		start_wait();
	}

private:
	void start_wait()
	{
		timer.async_wait(boost::bind(&PeriodicTask::execute
			, this
			, boost::asio::placeholders::error));
	}

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
		io_service.run();
	}

	void addTask(std::string const& name
		, PeriodicTask::handler_fn const& task
		, int interval)
	{
		tasks.push_back(std::make_unique<PeriodicTask>(std::ref(io_service)
			, name, interval, task));
	}

private:
	boost::asio::io_service io_service;
	std::vector<std::unique_ptr<PeriodicTask>> tasks;
};

int main()
{
	PeriodicScheduler scheduler;

	scheduler.addTask("CPU", boost::bind(log_text, "* CPU USAGE"), 5);
	scheduler.addTask("Memory1", boost::bind(log_text, "* MEMORY USAGE"), 10);
	scheduler.addTask("Memory2", boost::bind(log_text, "* MEMORY USAGE"), 10);
	scheduler.addTask("Memory3", boost::bind(log_text, "* MEMORY USAGE"), 10);
	scheduler.addTask("Memory4", boost::bind(log_text, "* MEMORY USAGE"), 10);
	scheduler.addTask("Memory5", boost::bind(log_text, "* MEMORY USAGE"), 10);
	scheduler.addTask("Memory6", boost::bind(log_text, "* MEMORY USAGE"), 10);
	scheduler.addTask("Memory7", boost::bind(log_text, "* MEMORY USAGE"), 10);
	scheduler.addTask("Memory8", boost::bind(log_text, "* MEMORY USAGE"), 10);

	log_text("Start io_service");

	scheduler.run();

	return 0;
}