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
 * Common networking functions for Bumblebee
 */
#pragma once

#define SOCK_BLOCK 0
#define SOCK_NOBLOCK 1

int socketConnect(char * address, int nonblock);
void socketClose(int * sock);
int socketCanRead(int sock);
int socketCanWrite(int sock);
int socketWrite(int * sock, void * buffer, int len);
int socketRead(int * sock, void * buffer, int len);
int socketServer(char * address, int nonblock);
int socketAccept(int * sock, int nonblock);
