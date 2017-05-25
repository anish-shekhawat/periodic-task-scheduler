/**
  C++ Multithreaded Periodic Task Scheduler Main Function
  
  main.cpp

  Purpose:	
  Defines task functions and initializes and runs the scheduler
  Provides menu driven functions to add, delete and update tasks

  @author Anish Singh Shekhawat
  @version 1.0 05/14/2017
*/
#include <Windows.h>
#include <Psapi.h>
#include "task_sqlite.h"
#include "PeriodicScheduler.h"
#include <boost\thread.hpp>

#define DBFILE "taskscheduler.db"

/**
  Task that returns the physical memory used by the process

  @param db Sqlite database connection pointer
*/
void physical_memory_usage(sqlite3 *db)
{	
	PROCESS_MEMORY_COUNTERS_EX pmc_ex;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc_ex, sizeof(pmc_ex));
	long physical_mem_usage = (long)pmc_ex.PrivateUsage;
	
	// Inserts the value into sqlite database
	insert_into_table(DBFILE, db, physical_mem_usage, "PHYSICAL_MEM");
}

/**
  Task that returns the virtual memory used by the process

  @param db Sqlite database connection pointer
*/
void virtual_memory_usage(sqlite3 *db)
{
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	long virtual_mem_used = (long)(memInfo.ullTotalPageFile - memInfo.ullAvailPageFile);
	
	// Inserts the value into sqlite database
	insert_into_table(DBFILE, db, fabs(virtual_mem_used), "VIRTUAL_MEM");
}

int main()
{
	sqlite3 *mainDB = NULL;
	initialize_database(DBFILE, mainDB);													//Initialize Database
	sqlite3_open_v2(DBFILE, &mainDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, NULL);	// Open DB connection

	PeriodicScheduler scheduler;

	// Schedule tasks initially
	scheduler.schedule_periodic(scheduler.getUid(), "VIRTUAL MEM USAGE", boost::bind(virtual_memory_usage, mainDB), std::chrono::system_clock::now(), 5);
	scheduler.schedule_periodic(scheduler.getUid(), "PHY MEM USAGE", boost::bind(physical_memory_usage, mainDB), std::chrono::system_clock::now(), 10);
	scheduler.schedule_periodic(scheduler.getUid(), "VIRTUAL MEM USAGE", boost::bind(virtual_memory_usage, mainDB), std::chrono::system_clock::now(), 7);
	scheduler.schedule_periodic(scheduler.getUid(), "PHY MEM USAGE", boost::bind(physical_memory_usage, mainDB), std::chrono::system_clock::now(), 15);
	scheduler.schedule_periodic(scheduler.getUid(), "VIRTUAL MEM USAGE", boost::bind(virtual_memory_usage, mainDB), std::chrono::system_clock::now(), 3);
	scheduler.schedule_periodic(scheduler.getUid(), "PHY MEM USAGE", boost::bind(physical_memory_usage, mainDB), std::chrono::system_clock::now(), 5);
	scheduler.schedule_periodic(scheduler.getUid(), "VIRTUAL MEM USAGE", boost::bind(virtual_memory_usage, mainDB), std::chrono::system_clock::now(), 10);
	scheduler.schedule_periodic(scheduler.getUid(), "PHY MEM USAGE", boost::bind(physical_memory_usage, mainDB), std::chrono::system_clock::now(), 4);

	// Run the scheduler in a new thread
	boost::thread th(&PeriodicScheduler::run, &scheduler);

	std::uint32_t taskid;
	int interval = 5;

	int option;

	//do-while loop to display menu until user select to exit program
	do
	{
		//Displaying Options for the menu
		std::cout << std::endl;
		std::cout << "1) Display Task List " << std::endl;						// Option to view task list
		std::cout << "2) Add new Task " << std::endl;							// Option to add new task
		std::cout << "3) Update Task Interval " << std::endl;					// Option to update an existing task
		std::cout << "4) Delete Task" << std::endl;								// Option to delete task
		std::cout << "5) Exit Program " << std::endl;							// Option to exit the program
		
		std::cout << "Please select an option : ";
		std::cin >> option;

		switch (option)
		{
		case 1:
			scheduler.get_tasks_overview();
			break;

		case 2:
			int task_option;
			std::cout << "Enter Task interval ";								// Get new task interval
			std::cin >> interval;									
			std::cout << "Enter Task type to schedule: " << std::endl;			// Ask user to choose task type
			std::cout << "1) Physical Memory Usage " << std::endl;
			std::cout << "2) Virtual Memory Usage " << std::endl;
			std::cout << "Please select an option : ";
			std::cin >> task_option;
			if (task_option == 1)
			{
				try {
					scheduler.schedule_periodic(scheduler.getUid(),				// Schedule task type 1
						"PHY MEM USAGE",
						boost::bind(physical_memory_usage, mainDB),
						std::chrono::system_clock::now(), interval);
				}
				catch (std::exception const &e) {
					std::cout << e.what() << std::endl;
				}
			}
			else if (task_option == 2)
			{
				try {
					scheduler.schedule_periodic(scheduler.getUid(),				// Schedule task type 2
						"VIRTUAL MEM USAGE",
						boost::bind(virtual_memory_usage, mainDB),
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
			scheduler.get_tasks_overview();										// Displays task list before prompting for task id
			std::cout << "Enter Task UID and new Interval separated by spaces to update ";
			std::cin >> taskid >> interval;										// Prompt user for new task interval
			scheduler.update_task(taskid, interval);							// Add task to update set
			break;

		case 4:
			std::uint32_t taskid;
			scheduler.get_tasks_overview();										// Displays task list before prompting for task id
			std::cout << "Enter Task UID to delete ";							
			std::cin >> taskid;													// Prompt user for task ID
			scheduler.delete_task(taskid);										// Add task to delete set
			break;

		case 5:
			scheduler.stop();													// Stop the scheduler
			break;

		default:
			std::cout << "Incorrect option entered!" << std::endl;
		}
	} while (option != 5);

	th.join();
	return 0;
}