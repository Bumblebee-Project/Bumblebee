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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "bbrun.h"
#include "bblogger.h"

int handler_set = 0;
int dowait = 1;

/// Socket list structure for use in main_loop.

struct pidlist {
  pid_t PID;
  struct pidlist *prev;
  struct pidlist *next;
};

struct pidlist * pidlist_start = 0; ///Begin of the linked-list of PIDs, if any.

/// Adds a pid_t to the linked list of PIDs.
/// Creates the list if it is still null.

static void pidlist_add(pid_t newpid) {
  struct pidlist *curr = malloc(sizeof (struct pidlist));
  curr->PID = newpid;
  curr->prev = 0;
  // the PID is inserted BEFORE the first PID, this should not matter
  curr->next = pidlist_start ? pidlist_start : 0;
  pidlist_start = curr;
}

/// Removes a pid_t from the linked list of PIDs.
/// Makes list null if empty.
static void pidlist_remove(pid_t rempid) {
  struct pidlist *curr;
  struct pidlist *next_iter;
  for (curr = pidlist_start; curr; curr = next_iter) {
    next_iter = curr->next;
    if (curr->PID == rempid) {
      if (curr->next) {
        curr->next->prev = curr->prev;
      }
      if (curr->prev) {
        curr->prev->next = curr->next;
      } else {
        pidlist_start = curr->next;
      }
      free(curr);
    }
  }
}//pidlist_remove

/// Finds a pid_t in the linked list of PIDs.
/// Returns 0 if not found, 1 otherwise.

static int pidlist_find(pid_t findpid) {
  struct pidlist *curr;
  for (curr = pidlist_start; curr; curr = curr->next) {
    if (curr->PID == findpid) {
      return 1;
    }
  }
  return 0;
}//pidlist_find

static void childsig_handler(int signum) {
  if (signum != SIGCHLD) {
    return;
  }
  int chld_stat = 0;
  /* Wait for the child to exit */
  pid_t ret = wait(&chld_stat);
  /* Log the child termination and return value */
  if (ret == -1) {
    bb_log(LOG_DEBUG, "SIGCHILD received, but wait failed with %s\n",
            strerror(errno));
  } else if (WIFEXITED(chld_stat)) {
    bb_log(LOG_DEBUG, "Process with PID %i returned code %i\n", ret,
            WEXITSTATUS (chld_stat));
  } else if (WIFSIGNALED(chld_stat)) {
    bb_log(LOG_DEBUG, "Process with PID %i terminated with %i\n", ret,
            WTERMSIG(chld_stat));
  }
  pidlist_remove(ret);
}//childsig_handler

static void check_handler(void) {
  // Set handler for this child process if not already
  if (handler_set == 0) {
    struct sigaction new_action;
    new_action.sa_handler = childsig_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGCHLD, &new_action, NULL);
    handler_set = 1;
  }
}//check_handler

static void bb_run_exec_detached(char **argv);

/**
 * Forks and runs the given application and waits for the process to finish
 *
 * @param argv The arguments values, the first one is the program
 * @param detached non-zero if the std in/output must be redirected to /dev/null, zero otherwise
 * @return Exit code of the program (between 0 and 255) or -1 on failure
 */
int bb_run_fork(char **argv, int detached) {
  int exitcode = -1;

  check_handler();
  /* Fork and attempt to run given application */
  pid_t pid = fork();
  if (pid == 0) {
    /* child process after fork */
    if (detached) {
      bb_run_exec_detached(argv);
    } else {
      bb_run_exec(argv);
    }
  } else if (pid > 0) {
    /* parent process after fork */
    int status = 0;

    bb_log(LOG_DEBUG, "Process %s started, PID %i.\n", argv[0], pid);
    pidlist_add(pid);

    if (waitpid(pid, &status, 0) != -1) {
      if (WIFEXITED(status)) {
        /* program exited normally, return status */
        exitcode = WEXITSTATUS(status);
      } else if (WIFSIGNALED(status)) {
        /* program was terminated by a signal */
        exitcode = 128 + WTERMSIG(status);
      }
    } else {
      bb_log(LOG_ERR, "waitpid(%i) faild with %s\n", pid, strerror(errno));
    }
    pidlist_remove(pid);
  } else {
    /* Fork failed */
    bb_log(LOG_ERR, "Process %s could not be started. fork() failed.\n", argv[0]);
  }

  /* could not determine return value */
  return exitcode;
}

/**
 * Forks and runs the given application, using an optional LD_LIBRARY_PATH. The
 * function then returns immediately.
 * stderr and stdout of the ran application is redirected to the parameter redirect.
 * stdin is redirected to /dev/null always.
 *
 * @param argv The arguments values, the first one is the program
 * @param ldpath The library path to be used if any (may be NULL)
 * @param redirect The file descriptor to redirect stdout/stderr to. Must be valid and open.
 * @return The childs process ID
 */
pid_t bb_run_fork_ld_redirect(char **argv, char *ldpath, int redirect) {
  check_handler();
  // Fork and attempt to run given application
  pid_t ret = fork();
  if (ret == 0) {
    if (ldpath && *ldpath) {
      char *current_path = getenv("LD_LIBRARY_PATH");
      /* Fork went ok, set environment if necessary */
      if (current_path) {
        char *ldpath_new = malloc(strlen(ldpath) + 1 + strlen(current_path) + 1);
        if (ldpath_new) {
          strcpy(ldpath_new, ldpath);
          strcat(ldpath_new, ":");
          strcat(ldpath_new, current_path);
          setenv("LD_LIBRARY_PATH", ldpath_new, 1);
          free(ldpath_new);
        } else {
          bb_log(LOG_WARNING, "Could not allocate memory for LD_LIBRARY_PATH\n");
        }
      } else {
        setenv("LD_LIBRARY_PATH", ldpath, 1);
      }
    }
    //open /dev/null for stdin redirect
    int devnull = open("/dev/null", O_RDWR);
    //fail silently on error, nothing we can do about it anyway...
    if (devnull >= 0){dup2(devnull, STDIN_FILENO);}
    //redirect stdout and stderr to the given filenum.
    dup2(redirect, STDOUT_FILENO);
    dup2(redirect, STDERR_FILENO);
    //ok, all ready, now actually execute
    bb_run_exec(argv);
  } else {
    if (ret > 0) {
      // Fork went ok, parent process continues
      bb_log(LOG_DEBUG, "Process %s started, PID %i.\n", argv[0], ret);
      pidlist_add(ret);
    } else {
      // Fork failed
      bb_log(LOG_ERR, "Process %s could not be started. fork() failed.\n", argv[0]);
      return 0;
    }
  }
  return ret;
}

/**
 * Forks and runs the given application, waits for a maximum of timeout seconds for process to finish.
 *
 * @param argv The arguments values, the first one is the application path or name
 */
void bb_run_fork_wait(char** argv, int timeout) {
  check_handler();
  // Fork and attempt to run given application
  pid_t ret = fork();
  if (ret == 0) {
    // Fork went ok, child process replace
    bb_run_exec(argv);
  } else {
    if (ret > 0) {
      // Fork went ok, parent process continues
      bb_log(LOG_DEBUG, "Process %s started, PID %i.\n", argv[0], ret);
      pidlist_add(ret);
      //sleep until process finishes or timeout reached
      int i = 0;
      while (bb_is_running(ret) && ((i < timeout) || (timeout == 0)) && dowait) {
        usleep(1000000);
        i++;
      }
      //make a single attempt to kill the process if timed out, without waiting
      if (bb_is_running(ret)) {
        bb_stop(ret);
      }
    } else {
      // Fork failed
      bb_log(LOG_ERR, "Process %s could not be started. fork() failed.\n", argv[0]);
      return;
    }
  }
  return;
}

/// Returns 1 if a process is currently running, 0 otherwise.

int bb_is_running(pid_t proc) {
  return pidlist_find(proc);
}

/// Stops the running process, if any.
void bb_stop(pid_t proc) {
  if (bb_is_running(proc)) {
    kill(proc, SIGTERM);
  }
}

/// Stops the running process, if any.
/// Does not return until successful.
/// Is always successful, eventually.
void bb_stop_wait(pid_t proc) {
  int i = 0;
  while (bb_is_running(proc)) {
    ++i;
    //the first 10 attempts, use SIGTERM
    if (i < 10) {
      kill(proc, SIGTERM);
    } else {
      //after that, use SIGKILL
      kill(proc, SIGKILL);
    }
    if (dowait) {
      usleep(1000000); //sleep up to a second, waiting for process
    } else {
      usleep(10000); //sleep only 10ms, because we are in a hurry
    }
  }
}

/**
 * Stops all the running processes, if any
 */
void bb_stop_all(void) {
  bb_log(LOG_DEBUG, "Killing all remaining processes.\n");
  /* keep killing the first program in the list until it's empty */
  while (pidlist_start) {
    bb_stop_wait(pidlist_start->PID);
  }
}

/**
 * Attempts to run the given application, replacing the current process
 * @param argv The program to be run
 */
void bb_run_exec(char **argv) {
  execvp(argv[0], argv);
  bb_log(LOG_ERR, "Error running \"%s\": %s\n", argv[0], strerror(errno));
  exit(errno);
}

/**
 * Attempts to run the given application, replacing the current process but
 * redirect all standard in/outputs to /dev/null.
 * @param argv The program to be run
 */
static void bb_run_exec_detached(char **argv) {
  int old_stderr, exec_err;
  bb_log(LOG_DEBUG, "Hiding stderr for execution of %s\n", argv[0]);
  /* Redirect all three standard file descriptors to /dev/null.
   * If daemonized, this already happened - but doing it
   * again doesn't hurt since this fork won't run forever anyway.
   */
  int devnull = open("/dev/null", O_RDWR);
  if (devnull < 0){
    bb_log(LOG_ERR, "Could not open /dev/null: %s\n", strerror(errno));
  }
  old_stderr = dup(STDERR_FILENO);
  dup2(devnull, STDIN_FILENO);
  dup2(devnull, STDOUT_FILENO);
  dup2(devnull, STDERR_FILENO);
  //done redirecting, do the exec
  execvp(argv[0], argv);
  /* note: the below lines are only executed if execvp fails */
  exec_err = errno;
  dup2(old_stderr, STDERR_FILENO);
  bb_log(LOG_ERR, "Error running \"%s\": %s\n", argv[0], strerror(exec_err));
  exit(exec_err);
}

/**
 * Cancels waiting for processes to finish - use when doing a fast shutdown.
 */
void bb_run_stopwaiting(void){
  dowait = 0;
}
