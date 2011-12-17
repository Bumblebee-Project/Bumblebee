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

int handler_set = 0;

/// Socket list structure for use in main_loop.
struct pidlist{
  pid_t PID;
  struct pidlist * next;
};

struct pidlist * pidlist_start = 0; ///Begin of the linked-list of PIDs, if any.

/// Adds a pid_t to the linked list of PIDs.
/// Creates the list if it is still null.
void pidlist_add(pid_t newpid){
  struct pidlist * curr = 0;
  curr = pidlist_start;
  //no list? create one.
  if (curr == 0){
    curr = malloc(sizeof(struct pidlist));
    curr->next = 0;
    curr->PID = newpid;
    pidlist_start = curr;
    return;
  }
  //find the last item in the list
  while ((curr != 0) && (curr->next != 0)){curr = curr->next;}
  //curr now holds the last item, curr->next is null
  curr->next = malloc(sizeof(struct pidlist));
  curr = curr->next;//move to new element
  curr->next = 0;
  curr->PID = newpid;
}//pidlist_add

/// Removes a pid_t from the linked list of PIDs.
/// Makes list null if empty.
void pidlist_remove(pid_t rempid){
  struct pidlist * curr = 0;
  struct pidlist * prev = 0;
  curr = pidlist_start;
  //no list? cancel.
  if (curr == 0){return;}
  //find the item in the list
  while (curr != 0){
    if (curr->PID == rempid){
      if (prev != 0){prev->next = curr->next;}
      if (curr == pidlist_start){pidlist_start = curr->next;}
      free(curr);
      if (prev != 0){
        curr = prev->next;
      }else{
        curr = 0;
      }
      continue;//just in case it was added twice for some reason
    }
    //go to the next item
    prev = curr;
    curr = curr->next;
  }
}//pidlist_remove

/// Finds a pid_t in the linked list of PIDs.
/// Returns 0 if not found, 1 otherwise.
int pidlist_find(pid_t findpid){
  struct pidlist * curr = 0;
  curr = pidlist_start;
  //no list? cancel.
  if (curr == 0){return 0;}
  //find the item in the list
  while (curr != 0){
    if (curr->PID == findpid){return 1;}
    curr = curr->next;
  }
  return 0;
}//pidlist_find

void childsig_handler(int signum){
  if (signum != SIGCHLD){return;}
  pid_t ret = wait(0);
  bb_log(LOG_DEBUG, "Process with PID %i terminated.\n", ret);
  pidlist_remove(ret);
}//childsig_handler

void check_handler(void){
  // Set handler for this child process if not already
  if (handler_set == 0){
    struct sigaction new_action;
    new_action.sa_handler = childsig_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGCHLD, &new_action, NULL);
    handler_set = 1;
  }
}//check_handler

/**
 * Forks and runs the given application.
 * More suitable for configurable arguments to pass
 *
 * @param argv The arguments values, the first one is the application path or name
 * @return The new process PID
 */
pid_t bb_run_fork(char** argv) {
  check_handler();
  // Fork and attempt to run given application
  pid_t ret = fork();
  if (ret == 0){
    // Fork went ok, child process replace
    bb_run_exec(argv);
  }else{
    if (ret > 0){
      // Fork went ok, parent process continues
      bb_log(LOG_INFO, "Process %s started, PID %i.\n", argv[0], ret);
      pidlist_add(ret);
    }else{
      // Fork failed
      bb_log(LOG_ERR, "Process %s could not be started. fork() failed.\n", argv[0]);
      return 0;
    }
  }
  return ret;

}

/**
 * Forks and runs the given application, waits for process to finish.
 *
 * @param argv The arguments values, the first one is the application path or name
 */
void bb_run_fork_wait(char** argv) {
  check_handler();
  // Fork and attempt to run given application
  pid_t ret = fork();
  if (ret == 0){
    // Fork went ok, child process replace
    bb_run_exec(argv);
  }else{
    if (ret > 0){
      // Fork went ok, parent process continues
      bb_log(LOG_INFO, "Process %s started, PID %i.\n", argv[0], ret);
      pidlist_add(ret);
      //sleep until process finishes
      while (bb_is_running(ret)){usleep(1000000);}
    }else{
      // Fork failed
      bb_log(LOG_ERR, "Process %s could not be started. fork() failed.\n", argv[0]);
      return;
    }
  }
  return;
}

/// Returns 1 if a process is currently running, 0 otherwise.
int bb_is_running(pid_t proc){
  return pidlist_find(proc);
}

/// Stops the running process, if any.
void bb_stop(pid_t proc){
  if (bb_is_running(proc)){kill(proc, SIGTERM);}
}

/// Stops all the running processes, if any.
void bb_stop_all(void){
  struct pidlist * curr = 0;
  curr = pidlist_start;
  //no list? cancel.
  if (curr == 0){return;}
  //kill the whole list
  while (curr != 0){
    kill(curr->PID, SIGTERM);
    curr = curr->next;
  }
}

/// Attempts to run the given application, replacing the current process
void bb_run_exec(char ** argv){
  execvp(argv[0], argv);
  bb_log(LOG_ERR, "Error running \"%s\": %s\n", argv[0], strerror(errno));
  exit(42);
}
