/**
  C++ Multithreaded Periodic Task Scheduler

  task_sqlite.cpp

  Purpose:
  Function implementations for database operations

  @author Anish Singh Shekhawat
  @version 1.0 05/15/2017
*/
#include <iostream>
#include "task_sqlite.h"
#include <cstring>
#include <vector>

// Function to initialize database and call function to create tables if not already created
int initialize_database(char *DBfile, sqlite3 *mainDB)
{
	// Try to open taskscheduler DB
	if (sqlite3_open_v2(DBfile, &mainDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, NULL))
	{
		// Failed - Create DB
		if (!createDB(DBfile, mainDB))
		{
			fprintf(stderr, "Fatal: Unable to create test DB!\n");
			return(0);
		}
	}
	sqlite3_close(mainDB);
	return 1;
}

// Function to create tables if not already created
int createDB(char *DBfile, sqlite3 *mainDB)
{
	sqlite3_stmt *stmt;
	int rc;
	char sql_str[1024];

	// Open connection to database
	rc = sqlite3_open_v2(DBfile, &mainDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	
	if (rc)
	{
		fprintf(stderr, "Can't create database:  %s\n", DBfile);
		sqlite3_close(mainDB);
		mainDB = NULL;
		return(0);
	}

	/*CREATE TABLES*/

	// Create Physical Memory Usage Table
	strcpy_s(sql_str, "CREATE TABLE PHYSICAL_MEM (");
	strcat_s(sql_str, "ID INTEGER PRIMARY KEY AUTOINCREMENT,");
	strcat_s(sql_str, "Val REAL);");

	if (SQLITE_OK != sqlite3_prepare_v2(mainDB, sql_str, -1, &stmt, 0)) 
	{
		fprintf(stderr, "Prepare error: %s", sqlite3_errmsg(mainDB));
		return(0);
	}
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
		{
			fprintf(stderr, "Step error (%d): %s", rc, sqlite3_errmsg(mainDB));
			return(0);
		}
	}
	sqlite3_finalize(stmt);

	// Create Virtual Memory Usage Table
	strcpy_s(sql_str, "CREATE TABLE VIRTUAL_MEM (");
	strcat_s(sql_str, "ID INTEGER PRIMARY KEY AUTOINCREMENT,");
	strcat_s(sql_str, "Val REAL);");

	if (SQLITE_OK != sqlite3_prepare_v2(mainDB, sql_str, -1, &stmt, 0)) 
	{
		fprintf(stderr, "Prepare error: %s", sqlite3_errmsg(mainDB));
		return(0);
	}
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
		{
			fprintf(stderr, "Step error (%d): %s", rc, sqlite3_errmsg(mainDB));
			return(0);
		}
	}
	sqlite3_finalize(stmt);

	// Create Aggregate Table to store aggregate values of each task type
	strcpy_s(sql_str, "CREATE TABLE AGGREGATES (");
	strcat_s(sql_str, "Task_Type VARCHAR(60),");
	strcat_s(sql_str, "Average REAL,");
	strcat_s(sql_str, "Minimum REAL,");
	strcat_s(sql_str, "Maximum REAL);");

	if (SQLITE_OK != sqlite3_prepare_v2(mainDB, sql_str, -1, &stmt, 0)) 
	{
		fprintf(stderr, "Prepare error: %s", sqlite3_errmsg(mainDB));
		return(0);
	}
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
		{
			fprintf(stderr, "Step error (%d): %s", rc, sqlite3_errmsg(mainDB));
			return(0);
		}
	}
	sqlite3_finalize(stmt);

	// Create an index on Aggregates Table
	strcpy_s(sql_str, "CREATE UNIQUE INDEX myindex ON AGGREGATES(Task_Type)");
	
	if (SQLITE_OK != sqlite3_prepare_v2(mainDB, sql_str, -1, &stmt, 0)) 
	{
		fprintf(stderr, "Prepare error: %s", sqlite3_errmsg(mainDB));
		return(0);
	}
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
		{
			fprintf(stderr, "Step error (%d): %s", rc, sqlite3_errmsg(mainDB));
			return(0);
		}
	}
	sqlite3_finalize(stmt);
	return(1);
}

// Function to insert task output in the database
int insert_into_table(char *DBfile, sqlite3 *DB, long usage, char *table)
{

	int rc;
	sqlite3_stmt *stmt;

	// Insert values in Database table according to task
	std::string x = "INSERT INTO " + std::string(table) + " (Val) VALUES (" + std::to_string(usage) + ")";

	const char *sql_str = x.c_str();

	if (SQLITE_OK != sqlite3_prepare_v2(DB, sql_str, -1, &stmt, 0)) 
	{
		fprintf(stderr, "Insert Prepare error: %s", sqlite3_errmsg(DB));
		return(0);
	}
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
		{
			fprintf(stderr, "Insert Step error (%d): %s", rc, sqlite3_errmsg(DB));
			return(0);
		}
	}
	sqlite3_finalize(stmt);

	// Call get_aggregates to get the new aggregated data after insert
	get_aggregates(DBfile, DB, table);

	return(1);
}

// Function to get aggregated data from the the task output table
void get_aggregates(char *DBfile, sqlite3 *DB, char *task)
{
	int rc;
	sqlite3_stmt *stmt;

	// Get Average, Minimum & Maximum values
	std::string str = "SELECT AVG(Val), MIN(Val), MAX(Val) from " + std::string(task); 
	
	const char *sql_str = str.c_str();
	const unsigned char *text1, *text2, *text3;
	std::string avg, min, max;

	
	if (SQLITE_OK != sqlite3_prepare_v2(DB, sql_str, -1, &stmt, 0)) 
	{
		fprintf(stderr, "GEt Prepare error: %s", sqlite3_errmsg(DB));
		return;
	}
	else
	{
		while (1)
		{
			rc = sqlite3_step(stmt);
			if (rc == SQLITE_ROW) 
			{
				text1 = sqlite3_column_text(stmt, 0);
				text2 = sqlite3_column_text(stmt, 1);
				text3 = sqlite3_column_text(stmt, 2);
				avg = std::string(reinterpret_cast<const char*>(text1));
				min = std::string(reinterpret_cast<const char*>(text2));
				max = std::string(reinterpret_cast<const char*>(text3));
			}
			else if (rc == SQLITE_DONE)
			{
				break;
			}			
			else
			{
				fprintf(stderr, "Wile Prepare error: %s", sqlite3_errmsg(DB));
				break;
			}
		}

	}
	sqlite3_finalize(stmt);

	// Insert these new aggregate values in the AGGREGATES Table
	insert_aggregates(DBfile, DB, avg, min, max, task);
}

// Function to insert aggregated data into the aggregate table according to the task
int insert_aggregates(char *DBfile, sqlite3 *DB, std::string avg, std::string min, std::string max, char *task)
{
	int rc;

	sqlite3_stmt *stmt;
	
	// Insert new row for the aggregate data of a task if not already present else replace it with new values
	// Uses Index on AGGREGATES Table to check if aggregate data of a task is already present
	std::string x = "INSERT OR REPLACE INTO AGGREGATES (Task_Type, Average, Minimum, Maximum) Values('" + std::string(task);
	x += "', " + avg;
	x += ", " + min;
	x += ", " + max;
	x += ")";
	
	const char *sql_str = x.c_str();
	
	
	if (SQLITE_OK != sqlite3_prepare_v2(DB, sql_str, -1, &stmt, 0)) {
		fprintf(stderr, "Replace Prepare error: %s", sqlite3_errmsg(DB));
		return 0;
	}
	else
	{
		rc = sqlite3_step(stmt);
		if (rc != SQLITE_DONE)
		{
			fprintf(stderr, "Step error (%d): %s", rc, sqlite3_errmsg(DB));
			return 0;

		}
	}
	sqlite3_finalize(stmt);
	return 1;
}