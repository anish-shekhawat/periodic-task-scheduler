#include<iostream>
#include<chrono>
#include<thread>
#include<boost\asio\io_service.hpp>
#include<boost\thread.hpp>

class ThreadPool
{
private:
	boost::asio::io_service io_service_;
	boost::asio::io_service::work work_;
	boost::thread_group threads_;
	std::size_t available_;
	boost::mutex mutex_;

public:
	ThreadPool(std::size_t pool_size)
		:work_(io_service_),
		available_(pool_size)
	{
		for ( std::size_t i = 0; i < pool_size; ++i )
		{
			threads_.create_thread( boost::bind( &boost::asio::io_service::run, &io_service_ ) );
		}
	}

	/// @brief Destructor.
	~ThreadPool()
	{
		// Force all threads to return from io_service::run().
		io_service_.stop();

		std::cout << "\nDestroying the class!\n";
		// Suppress all exceptions.
		try
		{
			threads_.join_all();
		}
		catch (const std::exception&) {}
	}

	/// @brief Adds a task to the thread pool if a thread is currently available.
	template < typename Task >
	void run_task(Task task)
	{
		boost::unique_lock< boost::mutex > lock(mutex_);

		// If no threads are available, then return.
		//if (0 == available_) return;

		// Decrement count, indicating thread is no longer available.
		--available_;
		std::cout << "\nAvailable Run: " << available_;
		// Post a wrapped task into the queue.
		io_service_.post(boost::bind(&ThreadPool::wrap_task, this,
			boost::function< void() >(task)));
	}

private:
	/// @brief Wrap a task so that the available count can be increased once
	///        the user provided task has completed.
	void wrap_task(boost::function< void() > task)
	{
		// Run the user supplied task.
		try
		{
			task();
		}
		// Suppress all exceptions.
		catch (const std::exception&) {}

		// Task has finished, so increment count of available threads.
		boost::unique_lock< boost::mutex > lock(mutex_);
		++available_;
		std::cout << "\nAvailable End: " << available_;
	}
};

void work1()
{
	std::cout << "\nTask 1 running\n";
	std::cout << "\nTask 1 Thread: " << boost::this_thread::get_id();;
	std::this_thread::sleep_for(std::chrono::seconds(5));
	std::cout << "\nTask 1 finished";
}

void work2()
{	
	std::cout << "\nTask 2 running: ";
	std::cout << "\nTask 2 Thread: " << boost::this_thread::get_id();;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	std::cout << "\nTask 2 finished\n";
}

void work3()
{
	std::cout << "\nTask 3 running";
	std::cout << "\nTask 3 Thread: " << boost::this_thread::get_id();;
	std::this_thread::sleep_for(std::chrono::seconds(3));
	std::cout << "\nTask 3 finished\n";
}

int main()
{
	ThreadPool pool(2);
	pool.run_task(work1);
	pool.run_task(work2);
	pool.run_task(work3);
	std::cout << "Done\n";
	return 0;
}