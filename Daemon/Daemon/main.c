#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "daemon.h"
#include "control_app.h"
#include "def.h"

int main(int argc, char *argv[])
{

    int erc = 0, isControlMode = 1;
    int pid;
    const char *daemonName;

    if (argc != 1 && argc != 2) {
        fprintf (stdout, "\nUSAGE: daemon_name [-d]\n\n");
        erc = 1;
        return erc;
    }

    if (argc == 2) {
        if (strcmp (argv[1], "-d") == 0) {
            isControlMode = 0;
        } else {
            fprintf (stdout, "\nError: Unrecognized parameter %s\n\n", argv[1]);
            erc = 2;
            return erc;
        }
    }

    if (getuid () != 0 && getgid () != 0) {
        fprintf (stdout, "\nError: Try to launch with root privileges\n\n");
        erc = 3;
        return erc;
    }

    if (extractDaemonFilename (argv[0], &daemonName) < 0) {

        fprintf (stderr, "\nError: Filename of daemon in the path (%s) wasn't defined\n\n", argv[0]);
        erc = -1;
        return erc;
    }

    if (!isControlMode) {
        pid = fork ();

        switch(pid) {
        case 0:
            erc = startDaemon (daemonName);
            if (IS_NOT_ERROR (erc)) {
                erc = execDaemon ();
            }
            exit(erc);
        case -1:
            fprintf(stderr, "\nError: Unable to create process by fork (%s)\n\n", strerror(errno));
            break;
        default:
            fprintf(stdout, "\nDaemon was started successfully (PID = %d)\n\n", pid);
            break;
        }

        return erc;
    } else {
        fprintf (stdout, "\nStart control application\n\n");
        erc = startControlApp (daemonName);
    }
    return erc;
}





