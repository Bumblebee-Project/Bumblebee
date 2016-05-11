/*
 * Copyright (c) 2011-2013, The Bumblebee Project
 * Author: Joaquín Ignacio Aramendía <samsagax@gmail.com>
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
 * C-coded version of the Bumblebee daemon and optirun.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <grp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <libkmod.h>
#ifdef WITH_PIDFILE
#ifdef HAVE_LIBBSD_020
#include <libutil.h>
#else
#include <bsd/libutil.h>
#endif
#endif
#include "bbconfig.h"
#include "bbsocket.h"
#include "bblogger.h"
#include "bbsecondary.h"
#include "bbrun.h"
#include "pci.h"
#include "driver.h"
#include "switch/switching.h"

/**
 * Change GID and umask of the daemon
 * @return EXIT_SUCCESS if the gid could be changed, EXIT_FAILURE otherwise
 */
static int bb_chgid(void) {
  /* Change the Group ID of bumblebee */
  struct group *gp;
  errno = 0;
  gp = getgrnam(bb_config.gid_name);
  if (gp == NULL) {
    int error_num = errno;
    bb_log(LOG_ERR, "%s\n", strerror(error_num));
    bb_log(LOG_ERR, "There is no \"%s\" group\n", bb_config.gid_name);
    return EXIT_FAILURE;
  }
  if (setgid(gp->gr_gid) != 0) {
    bb_log(LOG_ERR, "Could not set the GID of bumblebee: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  /* Change the file mode mask */
  umask(027);
  return EXIT_SUCCESS;
}

/**
 * Fork to the background, and exit parent.
 * @return EXIT_SUCCESS if the daemon could fork, EXIT_FAILURE otherwise. Note
 * that the parent exits and the child continues to run
 */
static int daemonize(void) {
  /* Fork off the parent process */
  pid_t bb_pid = fork();
  if (bb_pid < 0) {
    bb_log(LOG_ERR, "Could not fork to background\n");
    return EXIT_FAILURE;
  }

  /* If we got a good PID, then we can exit the parent process. */
  if (bb_pid > 0) {
    exit(EXIT_SUCCESS);
  }

  /* Create a new SID for the child process */
  pid_t bb_sid = setsid();
  if (bb_sid < 0) {
    bb_log(LOG_ERR, "Could not set SID: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  /* Change the current working directory */
  if ((chdir("/")) < 0) {
    bb_log(LOG_ERR, "Could not change to root directory: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  /* Reroute standard file descriptors to /dev/null */
  int devnull = open("/dev/null", O_RDWR);
  if (devnull < 0){
    bb_log(LOG_ERR, "Could not open /dev/null: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
  dup2(devnull, STDIN_FILENO);
  dup2(devnull, STDOUT_FILENO);
  dup2(devnull, STDERR_FILENO);
  close(devnull);
  return EXIT_SUCCESS;
}

/**
 *  Handle recieved signals - except SIGCHLD, which is handled in bbrun.c
 */
static void handle_signal(int sig) {
  static int sigpipes = 0;

  switch (sig) {
    case SIGHUP:
      bb_log(LOG_WARNING, "Received %s signal (ignoring...)\n", strsignal(sig));
      break;
    case SIGPIPE:
      /* if bb_log generates a SIGPIPE, i.e. when bumblebeed runs like
       * bumblebeed 2>&1 | cat and the pipe is killed, don't die infinitely */
      if (sigpipes <= 10) {
        bb_log(LOG_WARNING, "Received %s signal %i (signals 10> are ignored)\n",
                strsignal(sig), ++sigpipes);
      }
      break;
    case SIGINT:
    case SIGQUIT:
      bb_log(LOG_WARNING, "Received %s signal.\n", strsignal(sig));
      socketClose(&bb_status.bb_socket); //closing the socket terminates the server
      break;
    case SIGTERM:
      bb_log(LOG_WARNING, "Received %s signal.\n", strsignal(sig));
      socketClose(&bb_status.bb_socket); //closing the socket terminates the server
      bb_run_stopwaiting(); //speed up shutdown by not waiting for processes anymore
      break;
    default:
      bb_log(LOG_WARNING, "Unhandled signal %s\n", strsignal(sig));
      break;
  }
}

/// Socket list structure for use in main_loop.

struct clientsocket {
  int sock;
  int inuse;
  struct clientsocket * prev;
  struct clientsocket * next;
};

/// Receive and/or sent data to/from this socket.
/// \param sock Pointer to socket. Assumed to be valid.

static void handle_socket(struct clientsocket * C) {
  static char buffer[BUFFER_SIZE], *conf_key;
  bool need_secondary;
  //since these are local sockets, we can safely assume we get whole messages at a time
  int r = socketRead(&C->sock, buffer, BUFFER_SIZE);
  if (r > 0) {
    ensureZeroTerminated(buffer, r, BUFFER_SIZE);
    conf_key = strchr(buffer, ' ');
    switch (buffer[0]) {
      case 'S'://status
        if (bb_status.errors[0] != 0) {
          r = snprintf(buffer, BUFFER_SIZE, "Error (%s): %s\n", GITVERSION, bb_status.errors);
        } else {
          if (bb_is_running(bb_status.x_pid)) {
            r = snprintf(buffer, BUFFER_SIZE, "Ready (%s). X is PID %i, %i applications using bumblebeed.\n", GITVERSION, bb_status.x_pid, bb_status.appcount);
          } else {
            char *card_status;
            switch (switch_status()) {
              case SWITCH_OFF:
                card_status = "off";
                break;
              case SWITCH_ON:
                card_status = "on";
                break;
              default:
                /* no PM available, assume it's on */
                card_status = "likely on";
                break;
            }
            r = snprintf(buffer, BUFFER_SIZE, "Ready (%s). X inactive. Discrete"
                    " video card is %s.\n", GITVERSION, card_status);
          }
        }
        /* don't rely on result of snprintf, instead calculate length including
         * null byte. We assume a succesful write */
        socketWrite(&C->sock, buffer, strlen(buffer) + 1);
        break;
      case 'F'://force VirtualGL if possible
      case 'C'://check if VirtualGL is allowed
        need_secondary = conf_key ? strcmp(conf_key + 1, "NoX") : true;
        if (start_secondary(need_secondary)) {
          r = snprintf(buffer, BUFFER_SIZE, "Yes. X is active.\n");
          if (C->inuse == 0) {
            C->inuse = 1;
            bb_status.appcount++;
          }
        } else {
          if (bb_status.errors[0] != 0) {
            r = snprintf(buffer, BUFFER_SIZE, "No - error: %s\n", bb_status.errors);
          } else {
            r = snprintf(buffer, BUFFER_SIZE, "No, secondary X is not active.\n");
          }
        }
        /* don't rely on result of snprintf, instead calculate length including
         * null byte. We assume a succesful write */
        socketWrite(&C->sock, buffer, strlen(buffer) + 1);
        break;
      case 'D'://done, close the socket.
        socketClose(&C->sock);
        break;
      case 'Q': /* query for configuration details */
        /* required since labels can only be attached on statements */;
        if (conf_key) {
          conf_key++;
          if (strcmp(conf_key, "VirtualDisplay") == 0) {
            snprintf(buffer, BUFFER_SIZE, "Value: %s\n", bb_config.x_display);
          } else if (strcmp(conf_key, "LibraryPath") == 0) {
            snprintf(buffer, BUFFER_SIZE, "Value: %s\n", bb_config.ld_path);
          } else if (strcmp(conf_key, "Driver") == 0) {
            /* note: this is not the auto-detected value, but the actual one */
            snprintf(buffer, BUFFER_SIZE, "Value: %s\n", bb_config.driver);
          } else {
            snprintf(buffer, BUFFER_SIZE, "Unknown key requested.\n");
          }
        } else {
          snprintf(buffer, BUFFER_SIZE, "Error: invalid protocol message.\n");
        }
        socketWrite(&C->sock, buffer, strlen(buffer) + 1);
        break;
      default:
        bb_log(LOG_WARNING, "Unhandled message received: %s\n", buffer);
        break;
    }
  }
}

/* The main loop handles all connections and cleanup.
 * It returns if there are any problems with the listening socket.
 */
static void main_loop(void) {
  int optirun_socket_fd;
  struct clientsocket *client;
  struct clientsocket *last = 0; // the last client

  bb_log(LOG_INFO, "Initialization completed - now handling client requests\n");
  /* Listen for Optirun conections and act accordingly */
  while (bb_status.bb_socket != -1) {
    fd_set readfds;
    int max_fd = 0;

    FD_ZERO(&readfds);
#define FD_SET_AND_MAX(fd)                   \
    do if ((fd) >= 0 && (fd) < FD_SETSIZE) { \
      FD_SET((fd), &readfds);                \
      if (max_fd < (fd))                     \
        max_fd = (fd);                       \
    } while (0)
    FD_SET_AND_MAX(bb_status.bb_socket);
    FD_SET_AND_MAX(bb_status.x_pipe[0]);
    for (client = last; client; client = client->prev)
      FD_SET_AND_MAX(client->sock);
#undef FD_SET_AND_MAX

    if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
      if (errno == EINTR)
        continue;
      bb_log(LOG_ERR, "select() failed: %s\n", strerror(errno));
      break;
    }

#define FD_EVENT(fd) ((fd) >= 0 && FD_ISSET((fd), &readfds))
    if (FD_EVENT(bb_status.bb_socket)) {
      /* Accept a connection. */
      optirun_socket_fd = socketAccept(&bb_status.bb_socket, SOCK_NOBLOCK);
      if (optirun_socket_fd >= 0) {
        bb_log(LOG_DEBUG, "Accepted new connection\n", optirun_socket_fd, bb_status.appcount);

        /* add to list of sockets */
        client = malloc(sizeof (struct clientsocket));
        client->sock = optirun_socket_fd;
        client->inuse = 0;
        client->prev = last;
        client->next = 0;
        if (last) {
          last->next = client;
        }
        last = client;
      }
    }

    //check the X output pipe, if open
    if (FD_EVENT(bb_status.x_pipe[0]))
      check_xorg_pipe();

    /* loop through all connections, removing dead ones, receiving/sending data to the rest */
    struct clientsocket *next_iter;
    for (client = last; client; client = next_iter) {
      /* set the next client here because client may be free()'d */
      next_iter = client->prev;
      if (FD_EVENT(client->sock))
        handle_socket(client);
      if (client->sock < 0) {
        //remove from list
        if (client->inuse > 0) {
          bb_status.appcount--;
          //stop X / card if there is no need to keep it running
          if ((bb_status.appcount == 0) && (bb_config.stop_on_exit)) {
            stop_secondary();
          }
        }
        if (client->next) {
          client->next->prev = client->prev;
        } else {
          last = client->prev;
        }
        if (client->prev) {
          client->prev->next = client->next;
        }
        free(client);
      }
    }
#undef FD_EVENT
  }//socket server loop

  /* loop through all connections, closing all of them */
  client = last;
  while (client) {
    //close socket if not already closed
    if (client->sock >= 0) {
      socketClose(&client->sock);
    }
    //remove from list
    if (client->inuse > 0) {
      bb_status.appcount--;
    }
    // change the client here because after free() there is no way to know prev
    last = client;
    client = client->prev;
    free(last);
  }
  if (bb_status.appcount != 0) {
    bb_log(LOG_WARNING, "appcount = %i (should be 0)\n", bb_status.appcount);
  }
}

/**
 * Returns the option string for this program
 * @return An option string which can be used for getopt
 */
const char *bbconfig_get_optstr(void) {
  return BBCONFIG_COMMON_OPTSTR "Dx:g:m:k:";
}

/**
 * Returns the long options for this program
 * @return A option struct which can be used for getopt_long
 */
const struct option *bbconfig_get_lopts(void) {
  static struct option longOpts[] = {
    {"daemon", 0, 0, 'D'},
    {"xconf", 1, 0, 'x'},
    {"xconfdir", 1, 0, OPT_X_CONF_DIR_PATH},
    {"group", 1, 0, 'g'},
    {"module-path", 1, 0, 'm'},
    {"driver-module", 1, 0, 'k'},
    {"driver", 1, 0, OPT_DRIVER},
#ifdef WITH_PIDFILE
    {"pidfile", 1, 0, OPT_PIDFILE},
#endif
    {"use-syslog", 0, 0, OPT_USE_SYSLOG},
    {"pm-method", 1, 0, OPT_PM_METHOD},
    BBCONFIG_COMMON_LOPTS
  };
  return longOpts;
}

/**
 * Parses local command line options
 * @param opt The short option
 * @param value Value for the option if any
 * @return 1 if the option has been processed, 0 otherwise
 */
int bbconfig_parse_options(int opt, char *value) {
  switch (opt) {
    case OPT_USE_SYSLOG:
      /* already processed in bbconfig.c */
      break;
    case 'D'://daemonize
      bb_status.runmode = BB_RUN_DAEMON;
      break;
    case 'x'://xorg.conf path
      set_string_value(&bb_config.x_conf_file, value);
      break;
    case OPT_X_CONF_DIR_PATH://xorg.conf.d path
      set_string_value(&bb_config.x_conf_dir, value);
      break;
    case 'g'://group name to use
      set_string_value(&bb_config.gid_name, value);
      break;
    case 'm'://modulepath
      set_string_value(&bb_config.mod_path, value);
      break;
    case OPT_DRIVER://driver
      set_string_value(&bb_config.driver, value);
      break;
    case 'k'://kernel module
      set_string_value(&bb_config.module_name, value);
      break;
    case OPT_PM_METHOD:
      bb_config.pm_method = bb_pm_method_from_string(value);
      break;
#ifdef WITH_PIDFILE
    case OPT_PIDFILE:
      set_string_value(&bb_config.pid_file, value);
      break;
#endif
    default:
      /* no options parsed */
      return 0;
  }
  return 1;
}

int main(int argc, char* argv[]) {
#ifdef WITH_PIDFILE
  struct pidfh *pfh = NULL;
  pid_t otherpid;
#endif

  /* the logs needs to be ready before the signal handlers */
  init_early_config(argv, BB_RUN_SERVER);
  bbconfig_parse_opts(argc, argv, PARSE_STAGE_LOG);
  bb_init_log();

  /* Setup signal handling before anything else. Note that messages are not
   * shown until init_config has set bb_status.verbosity
   */
  signal(SIGHUP, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGINT, handle_signal);
  signal(SIGQUIT, handle_signal);
  signal(SIGPIPE, handle_signal);

  /* first load the config to make the logging verbosity level available */
  init_config();
  bbconfig_parse_opts(argc, argv, PARSE_STAGE_PRECONF);

  /* First look for an intel card */
  struct pci_bus_id *pci_id_igd = pci_find_gfx_by_vendor(PCI_VENDOR_ID_INTEL, 0);
  if (!pci_id_igd) {
    /* This is no Optimus configuration. But maybe it's a
       dual-nvidia configuration. Let us test that.
    */
    pci_id_igd = pci_find_gfx_by_vendor(PCI_VENDOR_ID_NVIDIA, 1);
    bb_log(LOG_INFO, "No Intel video card found, testing for dual-nvidia system.\n");

    if (!pci_id_igd) {
      /* Ok, this is not a double gpu setup supported (there is at most
         one nvidia and no intel cards */
      bb_log(LOG_ERR, "No integrated video card found, quitting.\n");
      return (EXIT_FAILURE);
    }
  }
  pci_bus_id_discrete = pci_find_gfx_by_vendor(PCI_VENDOR_ID_NVIDIA, 0);
  if (!pci_bus_id_discrete) {
    bb_log(LOG_ERR, "No discrete video card found, quitting\n");
    return (EXIT_FAILURE);
  }

  bb_log(LOG_DEBUG, "Found card: %02x:%02x.%x (discrete)\n", pci_bus_id_discrete->bus, pci_bus_id_discrete->slot, pci_bus_id_discrete->func);
  bb_log(LOG_DEBUG, "Found card: %02x:%02x.%x (integrated)\n", pci_id_igd->bus, pci_id_igd->slot, pci_id_igd->func);

  free(pci_id_igd);

  // kmod context have to be available for driver detection
  bb_status.kmod_ctx = kmod_new(NULL, NULL);
  if (bb_status.kmod_ctx == NULL) {
    bb_log(LOG_ERR, "kmod_new() failed!\n");
    bb_closelog();
    exit(EXIT_FAILURE);
  }

  GKeyFile *bbcfg = bbconfig_parse_conf();
  bbconfig_parse_opts(argc, argv, PARSE_STAGE_DRIVER);
  driver_detect();
  if (bbcfg) {
    bbconfig_parse_conf_driver(bbcfg, bb_config.driver);
    g_key_file_free(bbcfg);
  }
  bbconfig_parse_opts(argc, argv, PARSE_STAGE_OTHER);
  check_pm_method();

  /* dump the config after detecting the driver */
  config_dump();

  if (config_validate() != 0) {
    return (EXIT_FAILURE);
  }

#ifdef WITH_PIDFILE
  /* only write PID if a pid file has been set */
  if (bb_config.pid_file[0]) {
    pfh = pidfile_open(bb_config.pid_file, 0644, &otherpid);
    if (pfh == NULL) {
      if (errno == EEXIST) {
        bb_log(LOG_ERR, "Daemon already running, pid %d\n", otherpid);
      } else {
        bb_log(LOG_ERR, "Cannot open or write pidfile %s.\n", bb_config.pid_file);
      }
      bb_closelog();
      exit(EXIT_FAILURE);
    }
  }
#endif

  /* Change GID and mask according to configuration */
  if ((bb_config.gid_name != 0) && (bb_config.gid_name[0] != 0)) {
    int retval = bb_chgid();
    if (retval != EXIT_SUCCESS) {
      bb_closelog();
#ifdef WITH_PIDFILE
      pidfile_remove(pfh);
#endif
      exit(retval);
    }
  }

  bb_log(LOG_NOTICE, "%s %s started\n", bb_status.program_name, GITVERSION);

  /* Daemonized if daemon flag is activated */
  if (bb_status.runmode == BB_RUN_DAEMON) {
    int retval = daemonize();
    if (retval != EXIT_SUCCESS) {
      bb_closelog();
#ifdef WITH_PIDFILE
      pidfile_remove(pfh);
#endif
      exit(retval);
    }
  }

#ifdef WITH_PIDFILE
  /* write PID after daemonizing */
  pidfile_write(pfh);
#endif

  /* Initialize communication socket, enter main loop */
  bb_status.bb_socket = socketServer(bb_config.socket_path, SOCK_NOBLOCK);
  stop_secondary(); //turn off card, nobody is connected right now.
  main_loop();
  unlink(bb_config.socket_path);
  bb_status.runmode = BB_RUN_EXIT; //make sure all methods understand we are shutting down
  if (bb_config.card_shutdown_state) {
    //if shutdown state = 1, turn on card
    start_secondary(false);
  } else {
    //if shutdown state = 0, turn off card
    stop_secondary();
  }
  bb_closelog();
#ifdef WITH_PIDFILE
  pidfile_remove(pfh);
#endif
  bb_stop_all(); //stop any started processes that are left
  //close X pipe, if any parts of it are open still
  if (bb_status.x_pipe[0] != -1){close(bb_status.x_pipe[0]); bb_status.x_pipe[0] = -1;}
  if (bb_status.x_pipe[1] != -1){close(bb_status.x_pipe[1]); bb_status.x_pipe[1] = -1;}
  //cleanup kmod context
  kmod_unref(bb_status.kmod_ctx);
  return (EXIT_SUCCESS);
}
