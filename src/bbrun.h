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
#pragma once
#include <unistd.h>

/// Forks and runs the given application.
pid_t bb_run_fork(char** argv);

/// Forks and runs the given application, waits for process to finish.
void bb_run_fork_wait(char** argv);

/// Returns 1 if a process is currently running, 0 otherwise.
int bb_is_running(pid_t proc);

/// Stops the running process, if any.
void bb_stop(pid_t proc);

/// Stops all the running processes, if any.
void bb_stop_all(void);

/// Attempts to run the given application, replacing the current process
void bb_run_exec(char ** argv);

/// Adds a pid_t to the linked list of PIDs.
/// Creates the list if it is still null.
void pidlist_add(pid_t newpid);

/// Removes a pid_t from the linked list of PIDs.
/// Makes list null if empty.
void pidlist_remove(pid_t rempid);

/// Finds a pid_t in the linked list of PIDs.
/// Returns 0 if not found, 1 otherwise.
int pidlist_find(pid_t findpid);

/// TODO: doc function
void childsig_handler(int signum);

/// TODO: doc function
void check_handler(void);
