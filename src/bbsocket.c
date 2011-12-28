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

#include <sys/stat.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "bbsocket.h"
#include "bblogger.h"

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif


/// Create a new Unix Socket. This socket will (try to) connect to the given address right away.
/// \param address String containing the location of the Unix socket to connect to.
/// \param nonblock Whether the socket should be nonblocking. 1 means nonblocking, 0 means blocking.
/// \return An integer representing the socket, or -1 if connection failed.

int socketConnect(char * address, int nonblock) {
  //create the socket itself
  int sock = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    bb_log(LOG_ERR, "Could not create socket. Error: %s\n", strerror(errno));
    return -1;
  }
  //full our the address information for the connection
  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, address);
  //attempt to connect
  int r = connect(sock, (struct sockaddr*) & addr, sizeof (addr));
  if (r == 0) {
    //connection success, set nonblocking if requested.
    if (nonblock == 1) {
      int flags = fcntl(sock, F_GETFL, 0);
      flags |= O_NONBLOCK;
      fcntl(sock, F_SETFL, flags);
    }
  } else {
    //connection fail
    bb_log(LOG_ERR, "Could not connect to %s! Error: %s\n", address, strerror(errno));
    //close the socket and set it to -1
    socketClose(&sock);
  }
  return sock;
}//socketConnect

/// Nicely closes the given socket, setting it to -1.
/// Never fails.

void socketClose(int * sock) {
  bb_log(LOG_INFO, "Socket closed.\n");
  // do not attempt to close closed or uninitialized sockets
  if (!sock || *sock == -1) {
    return;
  }
  //half-close the socket first
  shutdown(*sock, SHUT_RDWR);
  //fully close the socket
  close(*sock);
  //set to -1 to prevent usage
  *sock = -1;
}//socketClose


/// Calls poll() on the socket, checking if data is available.
/// This function may return 1 even if there is no data, but never returns 0 when there is.
/// \return 1 if data can be read, 0 otherwise.

int socketCanRead(int sock) {
  if (sock < 0) {
    return 0;
  }
  struct pollfd PFD;
  PFD.fd = sock;
  PFD.events = POLLIN;
  PFD.revents = 0;
  poll(&PFD, 1, 5);
  if ((PFD.revents & POLLIN) == POLLIN) {
    return 1;
  } else {
    return 0;
  }
}//socketCanRead

/// Calls poll() on the socket, checking if data can be written.
/// \return 1 if data can be written, 0 otherwise.

int socketCanWrite(int sock) {
  if (sock < 0) {
    return 0;
  }
  struct pollfd PFD;
  PFD.fd = sock;
  PFD.events = POLLOUT;
  PFD.revents = 0;
  poll(&PFD, 1, 5);
  if ((PFD.revents & POLLOUT) == POLLOUT) {
    return 1;
  } else {
    return 0;
  }
}//socketCanWrite

/// Incremental write call. This function tries to write len bytes to the socket from the buffer,
/// returning the amount of bytes it actually wrote.
/// \param sock The socket to write to. Set to -1 if any error occurs.
/// \param buffer Location of the buffer to write from.
/// \param len Amount of bytes to write.
/// \returns The amount of bytes actually written.

int socketWrite(int * sock, void * buffer, int len) {
  if (*sock < 0) {
    return 0;
  }
  int r = send(*sock, buffer, len, 0);
  if (r < 0) {
    switch (errno) {
      case EWOULDBLOCK: return 0;
        break;
      default:
        bb_log(LOG_WARNING, "Could not write data! Error: %s\n", strerror(errno));
        socketClose(sock);
        return 0;
        break;
    }
  }
  if (r == 0) {
#if DEBUG >= 4
    //fprintf(stderr, "Could not iwrite data! Socket is closed.\n");
#endif
    socketClose(sock);
  }
  return r;
}//socketWrite

/// Incremental read call. This function tries to read len bytes to the buffer from the socket,
/// returning the amount of bytes it actually read.
/// \param sock The socket to read from. Set to -1 if any error occurs.
/// \param buffer Location of the buffer to read to.
/// \param len Amount of bytes to read.
/// \returns The amount of bytes actually read.

int socketRead(int * sock, void * buffer, int len) {
  if (*sock < 0) {
    return 0;
  }
  int r = recv(*sock, buffer, len, 0);
  if (r < 0) {
    switch (errno) {
      case EWOULDBLOCK: return 0;
        break;
      default:
        bb_log(LOG_WARNING, "Could not read data! Error: %s\n", strerror(errno));
        socketClose(sock);
        return 0;
        break;
    }
  }
  if (r == 0) {
    socketClose(sock);
  }
  return r;
}//socketRead

/// Create a new Unix Server. The socket is immediately bound and set to listen.
/// A maximum of 100 connections will be accepted between accept() calls.
/// Any further connections coming in will be dropped.
/// The address used will first be unlinked - so it succeeds if the Unix socket already existed. Watch out for this behaviour - it will delete any file located at address!
/// \param address The location of the Unix socket to bind to.
/// \param nonblock Whether accept() calls will be nonblocking. 0 = Blocking, 1 = Nonblocking.
/// \param return The socket itself, or -1 upon failure.

int socketServer(char * address, int nonblock) {
  //delete the file currently there, if any
  unlink(address);
  //create the socket
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0) {
    bb_log(LOG_ERR, "Could not create socket! Error: %s\n", strerror(errno));
    return -1;
  }
  //set to nonblocking if requested
  if (nonblock == 1) {
    int flags = fcntl(sock, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(sock, F_SETFL, flags);
  }
  //fill address information
  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  // XXX this path is 107 bytes (excl. null) on Linux, a larger path is
  // truncated. bb_config.socket_path can therefore be shrinked as well
  strncpy(addr.sun_path, address, sizeof(addr.sun_path) - 1);
  addr.sun_path[sizeof(addr.sun_path) - 1] = 0;
  //bind the socket
  int ret = bind(sock, (struct sockaddr*) & addr, sizeof (addr));
  if (ret == 0) {
    ret = listen(sock, 100); //start listening, backlog of 100 allowed
    //allow reading and writing for group and self
    chmod(address, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (ret != 0) {
      bb_log(LOG_ERR, "Listen failed! Error: %s\n", strerror(errno));
      socketClose(&sock);
    }
  } else {
    bb_log(LOG_ERR, "Binding failed! Error: %s\n", strerror(errno));
    socketClose(&sock);
  }
  return sock;
}//socketServer


/// Accept any waiting connections. If the Socket::Server is blocking, this function will block until there is an incoming connection.
/// If the Socket::Server is nonblocking, it might return a Socket::Connection that is not connected, so check for this.
/// \param nonblock Whether the newly connected socket should be nonblocking. Default is false (blocking).
/// \returns A valid socket or -1.

int socketAccept(int * sock, int nonblock) {
  if (*sock < 0) {
    return -1;
  }
  int r = accept(*sock, 0, 0);
  //set the socket to be nonblocking, if requested.
  //we could do this through accept4 with a flag, but that call is non-standard...
  if ((r >= 0) && (nonblock == 1)) {
    int flags = fcntl(r, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(r, F_SETFL, flags);
  }

  if (r < 0) {
    if ((errno != EWOULDBLOCK) && (errno != EAGAIN) && (errno != EINTR)) {
      bb_log(LOG_ERR, "Error during accept - closing server socket: %s\n", strerror(errno));
      socketClose(sock);
    }
  }
  return r;
}
