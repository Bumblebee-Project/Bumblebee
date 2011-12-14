/*
 * Copyright (C) 2011 Bumblebee Project
 * Author: Jaron Viëtor AKA "Thulinma" <jaron@vietors.com>
 *
 * This file is part of Bumblebee.
 *
 * Bumblebee is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Bumblebee is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Bumblebee. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Run command functions for Bumblebee
 */

#include "bbrun.h"
#include "bblogger.h"
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

pid_t curr_id = 0;
int handler_set = 0;

void childsig_handler(int signum){
  if (signum != SIGCHLD){return;}
  pid_t ret = wait(0);
  if (ret == curr_id){
    bb_log(LOG_INFO, "Process with PID %i terminated.\n", curr_id);
    curr_id = 0;
  }
}


/// Attempts to run the given command, replacing the current process
/// Use after forking! Never returns (exits on failure).
void runCmd(char * cmd){
  //split cmd into arguments
  //supports a maximum of 20 arguments
  char * tmp = cmd;
  char * tmp2 = 0;
  char * args[21];
  int i = 0;
  tmp2 = strtok(tmp, " ");
  args[0] = tmp2;
  while (tmp2 != 0 && (i < 20)){
    tmp2 = strtok(0, " ");
    ++i;
    args[i] = tmp2;
  }
  if (i == 20){args[20] = 0;}else{args[i+1] = 0;}
  //execute the command
  execvp(args[0], args);
  bb_log(LOG_ERR, "Error running \"%s\": %s\n", cmd, strerror(errno));
  exit(42);
}//runCmd

/// Attempts to run the given command with prefix, replacing the current process
void runCmd2(char * prefix, int argc, char ** argv){
  //split cmd into arguments
  //supports a maximum of 200 arguments
  char * tmp = prefix;
  char * tmp2 = 0;
  char * args[201];
  int i = 0;
  int j = 0;
  tmp2 = strtok(tmp, " ");
  args[0] = tmp2;
  while (tmp2 != 0 && (i < 200)){
    tmp2 = strtok(0, " ");
    ++i;
    args[i] = tmp2;
  }
  while ((j < argc) && (i < 200)){
    args[i] = argv[j];
    ++i;
    ++j;
  }
  if (i == 200){args[200] = 0;}else{args[i] = 0;}
  //execute the command
  execvp(args[0], args);
  bb_log(LOG_ERR, "Error running \"%s\": %s\n", args[0], strerror(errno));
  exit(42);
}//runCmd with prefix


/// Attempts to run the given command after forking.
/// Returns 0 on failure, the PID of the running application otherwise.
pid_t runFork(char * cmd){
  if (handler_set == 0){
    struct sigaction new_action;
    new_action.sa_handler = childsig_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGCHLD, &new_action, NULL);
    handler_set = 1;
  }
  if (isRunning()){
    bb_log(LOG_ERR, "Attempted to start a process while one was already running.\n");
    return 0;
  }
  pid_t ret = fork();
  printf("Mehness %i!\n", ret);
  if (ret == 0){
    runCmd(cmd);
  }else{
    if (ret > 0){
      bb_log(LOG_INFO, "Process %s started, PID %i.\n", cmd, ret);
      curr_id = ret;
    }else{
      bb_log(LOG_ERR, "Process %s could not be started. fork() failed.\n", cmd);
      return 0;
    }
  }
  return ret;
}//runFork

/**
 * Forks and run the given application.
 * More suitable for configurable arguments to pass
 *
 * @param argc The amount of argument passed
 * @param argv The arguments values, the first one is the full application path
 * @return The new process PID
 */
pid_t bb_run_fork(int argc, char** argv) {
  // Set handler for this child process if not already
  if (handler_set == 0){
    struct sigaction new_action;
    new_action.sa_handler = childsig_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGCHLD, &new_action, NULL);
    handler_set = 1;
  }
  if (isRunning()){
    bb_log(LOG_ERR, "Attempted to start a process while one was already running.\n");
    return 0;
  }
  // Fork and attempt to run given application
  pid_t ret = fork();
  if (ret == 0){
    // Fork went ok, child process replace
    bb_run_exec(argc, argv);
  }else{
    if (ret > 0){
      // Fork went ok, parent process continues
      bb_log(LOG_INFO, "Process %s started, PID %i.\n", cmd, ret);
      curr_id = ret;
    }else{
      // Fork failed
      bb_log(LOG_ERR, "Process %s could not be started. fork() failed.\n", cmd);
      return 0;
    }
  }
  return ret;

}

/// Returns 1 if a process is currently running, 0 otherwise.
int isRunning(){
  if (curr_id == 0){return 0;}
  return 1;
}

/// Stops the running process, if any.
void runStop(){
  if (isRunning()){kill(curr_id, SIGTERM);}
}

/// Attempts to run the given application, replacing the current process
void bb_run_exec(int argc, char ** argv){
  execvp(argv[0], argv);
  bb_log(LOG_ERR, "Error running \"%s\": %s\n", argv[0], strerror(errno));
  exit(42);
}

/// Attempts to run the given application with prefix, returning after the application finishes.
void runApp2(char * prefix, int argc, char ** argv){
  if (handler_set == 0){
    struct sigaction new_action;
    new_action.sa_handler = childsig_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGCHLD, &new_action, NULL);
    handler_set = 1;
  }
  if (isRunning()){
    bb_log(LOG_ERR, "Attempted to start a process while one was already running.\n");
    return;
  }
  pid_t ret = fork();
  if (ret == 0){
    runCmd2(prefix, argc, argv);
  }else{
    if (ret > 0){
      bb_log(LOG_INFO, "Process %s started, PID %i.\n", prefix, ret);
      curr_id = ret;
      //sleep until process finishes
      while (curr_id != 0){usleep(1000000);}
    }else{
      bb_log(LOG_ERR, "Process %s could not be started. fork() failed.\n", prefix);
    }
  }
  return;
}
