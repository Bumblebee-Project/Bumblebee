/*
 * Copyright (C) 2011 Bumblebee Project
 * Author: Joaquín Ignacio Aramendía <samsagax@gmail.com>
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
 * C-coded version of the Bumblebee daemon.
 */

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <grp.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "bbcommon.h"



/* Default buffer size */
static const size_t DEFAULT_BUFFER_SIZE = 256;

/* Structure containing the daemon configuration and status */
static struct {
    /* The name which the program was called */
    char* program_name;
    int is_daemonized;

    /* Process ID and Session ID */
    pid_t pid, sid;
    /* Communication socket */
    struct {
        int bb_sock_fd;
        struct sockaddr_un bb_socket_name;
    } bb_socket;

    /* Have track of their children */
    struct {
        int have_children;
        pid_t x_pid;
    } x_stat;
} bb_config;


/* Print a little note on usage */
static void print_usage(int exit_val) {
    // Print help message and exit with exit code
    printf("Usage: %s [options]\n", bb_config.program_name);
    printf("  Options:\n");
    printf("      -d\tRun as daemon.\n");
    printf("      -v\tBe verbose.\n");
    printf("      -h\tShow this help screen.\n");
    printf("\n");
    exit(exit_val);
}

/* Initialize log capabilities. Return 0 on success */
static int init_log(int daemon) {
    /* Open Logggin mechanism based on configuration */
    if (daemon) {
        openlog(DAEMON_NAME, LOG_PID, LOG_DAEMON);
    } else {
    }
    /* Should end with no error by now */
    return 0;
}

/*
 * Log a message to the current log mechanism.
 * Try to keep log messages less than 80 characters.
 */
static void bb_log(int priority, char* msg_format, ...) {
    va_list args;
	va_start(args, msg_format);
	if (bb_config.is_daemonized) {
	    vsyslog(priority, msg_format, args);
	} else {
	    char* fullmsg_fmt = malloc(DEFAULT_BUFFER_SIZE);
	    switch (priority) {
	        case LOG_ERR:
	            fullmsg_fmt = strcpy(fullmsg_fmt, "[ERROR]");
	            break;
	        case LOG_DEBUG:
	            fullmsg_fmt = strcpy(fullmsg_fmt, "[DEBUG]");
	            break;
	        case LOG_WARNING:
	            fullmsg_fmt = strcpy(fullmsg_fmt, "[WARN]");
	            break;
	        default:
	            fullmsg_fmt = strcpy(fullmsg_fmt, "[INFO]");
	    }
	    fullmsg_fmt = strcat(fullmsg_fmt, msg_format);
	    //Append NL char
	    fullmsg_fmt = strcat(fullmsg_fmt, "\n");
	    vfprintf(stderr, fullmsg_fmt, args);
	    free(fullmsg_fmt);
	}
	va_end(args);
}

/* close logging mechanism */
static void bb_closelog(void) {
    if (bb_config.is_daemonized) {
        closelog();
    } else {
    }
}

/* Start the X server by fork-exec */
static int start_x(void) {
    bb_log(LOG_INFO, "Dummy: Starting X server");
}

/* Kill the second X server if any */
static int stop_x(void) {
    bb_log(LOG_INFO, "Dummy: Stopping X server");
}


/* Set a PID file lock */
static int init_pidfile_lock(void) {
    //TODO:Error handling
    int lfp = open(PID_FILE, O_RDWR | O_CREAT, 0644);
    if(lfp != -1) {
        /* Lock entire file */
        struct flock lock = {
            .l_type = F_WRLCK,
            .l_start = 0,
            .l_whence = SEEK_SET,
            .l_len = 0,
            .l_pid = getpid()
        };
        if(fcntl(lfp, F_SETLK, &lock) != -1) {
            if(ftruncate(lfp, 0) == 0) {
                char pidtext[DEFAULT_BUFFER_SIZE];
                snprintf(pidtext, sizeof(pidtext), "%ld\n", getpid());
                int r = write(lfp, pidtext, strlen(pidtext));
                if(r != -1) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

/* Initialize a local socket for communication with optirun instances */
static int init_socket(void) {
    bb_log(LOG_INFO, "Opening communication socket.");

    /* Create the socket */
    bb_config.bb_socket.bb_sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if ( bb_config.bb_socket.bb_sock_fd == -1 ) {
        int error_number = errno;
        switch (error_number) {
            case EAFNOSUPPORT:
            case EMFILE:
            case EPROTONOSUPPORT:
            case EPROTOTYPE:
                bb_log(LOG_ERR, "Error in socket socket init: %s",
                        strerror(error_number));
                break;
            case EACCES:
            case ENOBUFS:
            case ENOMEM:
                bb_log(LOG_ERR, "Error in socket socket init: %s",
                        strerror(error_number));
                break;
            default:
                bb_log(LOG_ERR, "Unexpected error (bug)");
        }
        return (EXIT_FAILURE);
    }

    //TODO: Handle errors
    bb_config.bb_socket.bb_socket_name.sun_family = AF_LOCAL;
    strcpy(bb_config.bb_socket.bb_socket_name.sun_path, BBS_PATH);
    int bind_res = bind(bb_config.bb_socket.bb_sock_fd,
            (struct sockaddr *) &(bb_config.bb_socket.bb_socket_name),
            SUN_LEN (&(bb_config.bb_socket.bb_socket_name)));
    if (bind_res == -1) {
        bb_log(LOG_ERR, "Error in bind");
        return EXIT_FAILURE;
    }
    //Listen a maximum of 5 connections in queve
    int list_res = listen(bb_config.bb_socket.bb_sock_fd, 5);
    if (list_res == -1) {
        bb_log(LOG_ERR, "Error in bind");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/* Remove the socket file */
static void bb_close_socket(void) {
    if (close(bb_config.bb_socket.bb_sock_fd) == -1) {
        int err = errno;
        switch(err) {
            case EBADF:
            case EINTR:
            case EIO:
                bb_log(LOG_ERR, "%s", strerror(err));
        }
    }
    //TODO: Handle errors
    unlink (bb_config.bb_socket.bb_socket_name.sun_path);
    bb_log(LOG_INFO, "Socket closed");
}

/* Called when server must die */
static void die_gracefully() {
    /* Release all used resources, as quicly as we can */
    bb_close_socket();
    remove(PID_FILE);
    bb_closelog();
    exit(EXIT_SUCCESS);
}

/* Read the configuration file */
static int read_configuration() {
    FILE *cf = fopen(CONFIG_FILE, "r");
    if (cf==(NULL)) { /* An error ocurred */
        int err_num = errno;
        assert(cf == NULL);
        switch (err_num) {
            case EACCES:
            case EINVAL:
            case EIO:
            case EISDIR:
            case ELOOP:
            case EMFILE:
            case ENAMETOOLONG:
            case ENFILE:
            case ENOSR:
            case ENOTDIR:
                syslog(LOG_ERR, "Error in config file: %s", strerror(err_num));
        }
    }
    fclose(cf);
}

/* Fork to the background, and exit parent. */
static int daemonize(void) {

    /* Daemon flag, should be reset to zero if fail to daemonize */
    bb_config.is_daemonized = 1;

    //TODO: CHANGE GID ON DAEMON
    /* Change the Group ID of bumblebee */
    struct group *gp;
    errno = 0;
    gp = getgrnam(DEFAULT_BB_GROUP);
    if (gp == NULL) {
        int error_num = errno;
        bb_log(LOG_ERR, "%s", strerror(error_num));
        bb_log(LOG_ERR, "There is no \"%s\" group", DEFAULT_BB_GROUP);
        exit(EXIT_FAILURE);
    }
    if (setgid((*gp).gr_gid) != -1) {
        bb_log(LOG_ERR, "Could not set the GID of bumblebee");
        exit(EXIT_FAILURE);
    }

    /* Fork off the parent process */
    bb_config.pid = fork();
    if (bb_config.pid < 0) {
        bb_config.is_daemonized = 0;
        bb_log(LOG_ERR, "Could not fork to background");
        exit (EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (bb_config.pid > 0) {
        exit (EXIT_SUCCESS);
    }

    /* set PID lock file */
    if (init_pidfile_lock()) {
        bb_log(LOG_ERR, "Could not create PID file.");
        exit (EXIT_FAILURE);
    }

    /* Change the file mode mask */
    umask(023);

    /* Create a new SID for the child process */
    bb_config.sid = setsid();
    if (bb_config.sid < 0) {
        /* Log the failure */
        bb_config.is_daemonized = 0;
        bb_log(LOG_ERR, "Could not set SID: ", strerror(errno));
        exit (EXIT_FAILURE);
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        /* Log the failure */
        bb_config.is_daemonized = 0;
        bb_log(LOG_ERR, "Could not change to root directory: ", strerror(errno));
        exit (EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

/* Handle recieved signals */
static void handle_signal(int sig) {
    switch(sig) {
        case SIGHUP:
            bb_log(LOG_WARNING, "Received %s signal (ignoring...)", strsignal(sig));
            break;
        case SIGINT:
        case SIGQUIT:
        case SIGTERM:
            bb_log(LOG_WARNING, "Received %s signal.", strsignal(sig));
            die_gracefully();
            break;
        default:
            bb_log(LOG_WARNING, "Unhandled signal (%d) %s", strsignal(sig));
            break;
    }
}

/* Handle children (?) */
static void handle_children(int sig) {

}

/* Handle an Optirun connection */
static int handle_optirun(int optirun_socket_fd) {
    bb_log(LOG_INFO, "Optirun instance connection accepted");

    pid_t pid;
    pid = fork();
    if (pid < 0) {
        /* Error in fork handling connection */
    }
    if (pid == 0) {
        /* Child process to handle optirun connection */
        close(bb_config.bb_socket.bb_sock_fd);
        /* Handle the connection */

        /* Exit child */
        exit(0);
    } else {
        /* Close our end of the connection on parent process */
        close(optirun_socket_fd);
    }
    return 0;
}

/* Big Fat Loop. Never returns */
static void main_loop(void) {
    struct sockaddr_un optirun_name;
    socklen_t optirun_name_len;
    int optirun_socket_fd;

    bb_log(LOG_INFO, "Started main loop");
    /* Listen for Optirun conections and act accordingly */
    for(;;) {
        /* Accept a connection. */
        optirun_socket_fd = accept(bb_config.bb_socket.bb_sock_fd,
                               (struct sockaddr *) &optirun_name, &optirun_name_len);
        handle_optirun(optirun_socket_fd);
        bb_log(LOG_INFO, "Not waiting anymore, I'm a full server and I'm alive! YAY!" );
    }
}


int main(int argc, char* argv[]) {

    /* Setup signal handling before anything else */
    signal(SIGHUP, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);
    signal(SIGQUIT, handle_signal);
    signal(SIGCHLD, SIG_IGN);

    /* TODO: Should check for PID lock, we allow only one instance */

    /* Initializing configuration */
    bb_config.program_name = argv[0];
    bb_config.is_daemonized = 0;
    bb_config.x_stat.have_children = 0;

    /* Parse the options, set flags as necessary */
    int daemon = 0;
    int verbose = 0;
    int c;
    while( (c = getopt(argc, argv, "dvh|help")) != -1) {
        switch(c){
            case 'h':
                print_usage(EXIT_SUCCESS);
                break;
            case 'd':
                daemon = 1;
                break;
            case 'v':
                verbose = 1;
                fprintf(stderr, "Warning: Verbose mode not yet implemented\n");
                break;
            default:
                // Unrecognized option
                print_usage(EXIT_FAILURE);
                break;
        }
    }

    /* Init log Mechanism */
    if (init_log(daemon)) {
        fprintf(stderr, "Unexpected error, could not initialize log.");
        return 1;
    }

    /* Daemonized if daemon flag is activated */
    if (daemon) {
        if (daemonize()) {
            bb_log(LOG_ERR, "Error: Bumblebee could not be daemonized");
            exit(EXIT_FAILURE);
        }
    }

    /* Initialize communication socket */
    if (init_socket()) {
        bb_log(LOG_ERR, "Could not initialize Communication socket. Exit.\n");
        die_gracefully();
        exit(EXIT_FAILURE);
    }

    main_loop();

    return (EXIT_SUCCESS); /* Will never, ever reach this line */
}
