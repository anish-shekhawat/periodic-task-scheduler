Periodic Task Scheduler
=================================

Implements a generic, periodic task scheduler in C++ (not plain C). Each task runs on a separate, configurable interval (e.g., every 30 seconds). It can execute any type of task, where a task is just an abstraction for a block of code that when run, produces some output. Includes functions to accept new tasks, cancel tasks, and change the schedule of tasks.

The output of each task will be one or more "metrics" in the form of decimal values. The raw metric data and some aggregate metrics (such as average, minimum, and maximum) are stored in a SQLite database. The aggregate metrics are kept up-to-date for each new data point the program collects. If the program is run multiple times, it continues where it left off, augmenting the existing data.
