/**
  C++ Multithreaded Periodic Task Scheduler

  task_sqlite.h

  Purpose:
  Header file for database functions

  @author Anish Singh Shekhawat
  @version 1.0 05/15/2017
*/
#pragma once
#include <sqlite3.h>
#include <string>

/**
  Function to initialize database and call function to create tables if not already created

  @param DBfile Database File pointer
  @param mainDB Sqlite Database connection pointer
  @return returns 1 if successfull else 0
*/
int initialize_database(char *DBfile, sqlite3 *mainDB);

/**
  Function to create tables if not already created

  @param DBfile Database File pointer
  @param mainDB Sqlite Database connection pointer
  @return returns 1 if successfull else 0
*/
int createDB(char *DBfile, sqlite3 *mainDB);

/**
  Function to insert task output in the database

  @param DBfile Database File pointer
  @param mainDB Sqlite Database connection pointer
  @param usage Task output data
  @param table Task specific table to insert output in DB
  @return returns 1 if successfull else 0
*/
int insert_into_table(char *DBfile, sqlite3 *mainDB, long usage, char *table);

/**
  Function to get aggregated data from the the task output table 

  @param DBfile Database File pointer
  @param DB Sqlite Database connection pointer
  @param task Task name whose aggregated data is to be retrived
*/
void get_aggregates(char *DBfile, sqlite3 *DB, char *task);

/**
  Function to insert aggregated data into the aggregate table according to the task

  @param DBfile Database File pointer
  @param DB Sqlite Database connection pointer
  @param avg Average aggregate value
  @param min Minimum aggregate value
  @param max Maximum aggregate value
  @return returns 1 if successfull else 0
*/
int insert_aggregates(char *DBfile, sqlite3 *DB, std::string avg, std::string min, std::string max, char *task);