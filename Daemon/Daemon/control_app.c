#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "control_app.h"
#include "io_operations.h"
#include "log_manager.h"
#include "def.h"
#include "daemon.h"

pid_t dpid;
struct sigaction sa;
const char *daemonInputName;

int startControlApp (const char *daemonName) {
    int erc = NO_ERROR;
    daemonInputName = daemonName;

    fprintf (stdout, "\n");

    if (IS_NOT_ERROR (erc)) {
        erc = configureSignalControlAppHandlers ();
        if (HAS_ERROR (erc)) {
            fprintf (stdout, "Ocurred error during configure signals. %s\n", strerror(errno));
            return erc;
        }
    }

    if (IS_NOT_ERROR (erc)) {

        erc = setLogDaemonName (daemonName);

        switch (erc) {
        case -1:
            fprintf (stdout, "Daemon name is NULL\n");
            return erc;
        default:
            fprintf (stdout, "Preliminary initialization finished successfully\n");
            break;
        }
    }

    if (IS_NOT_ERROR (erc)) {

        erc = setPIDFilename(getDaemonName ());

        switch (erc) {
        case -2:
            fprintf (stdout, "The sum of string's length (%s) + (%s) too large\n",
                     getPIDFullFilename (), getPIDFilename ());
            return erc;
        case -1:
            fprintf(stdout, "Daemon name is NULL\n");
            return erc;
        default:
            //fprintf (stdout, "'%s' PID filename (%s)\n", getDaemonName (), getPIDFullFilename ());
            break;
        }
    }

    if (IS_NOT_ERROR (erc)) {
        erc = readPIDFromFile (&dpid);

        switch (erc) {
        case -2:
            fprintf (stdout, "Occured error during reading PID from the file (%s). %s\n",
                     getPIDFullFilename (), strerror(errno));
            return erc;
        case -1:
            fprintf (stdout, "Couldn't open the file '%s'. %s\n", getPIDFullFilename (), strerror(errno));
            fprintf (stdout, "Possibly the '%s' wasn't started\n", getDaemonName ());
            break;
        case 0:
            fprintf (stdout, "'%s' is executing with PID = %d\n", getDaemonName (), dpid);
            break;
        default:
            fprintf (stdout, "Unknown error code = %d returned by readPIDFromFile\n", erc);
            erc = -2; // Unknown error
            return erc;
        }
    }

    erc = mainControlAppLoop ();

    return erc;
}

void restartDaemonManual (const char *daemonName) {
    int erc = 0;
    pid_t pid;

    pid = fork ();

    switch(pid) {
    case 0:
        erc = startDaemon (daemonName);
        if (IS_NOT_ERROR (erc)) {
            erc = execDaemon ();
        }
        exit(erc);
    case -1:
        erc = -1;
        fprintf(stderr, "Unable to create process by fork (%s)\n", strerror(errno));
        break;
    default:
        fprintf(stdout, "Daemon was started successfully (PID = %d)\n", pid);
        break;
    }
}

int mainControlAppLoop () {

    char str[64];
    int erc = NO_ERROR, erc2;
    short isContinue = TRUE;
    struct timespec req, rem;

    while (isContinue) {
        req.tv_sec  = 0;
        req.tv_nsec = 150000000; //0.15 sec
        fprintf (stdout, "[a: Send SIG 'isAlive daemon', r: Restart daemon, q: Quit daemon, e: Exit]: ");
        fscanf (stdin, "%s", str);

        switch (str[0]) {
            case 'e':
                isContinue = FALSE;
                continue;
            case 'a':
            case 'r':
            case 'q':
                erc = readPIDFromFile (&dpid);

                switch (erc) {
                case -2:
                    fprintf (stdout, "Occured error during reading PID from the file (%s). %s\n",
                             getPIDFullFilename (), strerror(errno));
                    break;
                case -1:
                    if (str[0] == 'a' || str[0] == 'q') {
                        fprintf (stdout, "Couldn't open the file '%s'. %s\n", getPIDFullFilename (), strerror(errno));
                    }

                    if (str[0] == 'r') {
                        restartDaemonManual (daemonInputName);
                    }
                    break;
                case 0:
                    if (str[0] == 'a') {
                        printCurrentTimeToStdout ();
                        fprintf (stdout, "Send isAlive signal to the process (PID = %d)\n", dpid);
                        erc = kill (dpid, SIGINT);
                        req.tv_sec  = 2;
                        req.tv_nsec = 0;
                    }

                    if (str[0] == 'r') {
                        printCurrentTimeToStdout ();
                        fprintf (stdout, "Send Restart signal to the process (PID = %d)\n", dpid);
                        erc = kill (dpid, SIGHUP);
                    }

                    if (str[0] == 'q') {
                        printCurrentTimeToStdout ();
                        fprintf (stdout, "Send Quit signal to the process (PID = %d)\n", dpid);
                        erc = kill (dpid, SIGQUIT);
                    }
                    break;
                }
                break;
            default:
                break;
        }

        while (TRUE) {
            erc2 = nanosleep (&req, &rem);
            if (erc2 == -1) { // Interrupted by signal
                if (rem.tv_sec > 0 || rem.tv_nsec > 0) {
                    req.tv_sec  = rem.tv_sec + 1;
                    req.tv_nsec = rem.tv_nsec;
                    continue;
                }
            }
            break;
        }
    }

    return erc;
}

void printCurrentTimeToStdout () {
    time_t t = time(NULL);
    struct tm* _tm = localtime(&t);

    fprintf (stdout, "[%04d/%02d/%02d %02d:%02d:%02d] ", _tm->tm_year+1900, _tm->tm_mon+1, _tm->tm_mday, _tm->tm_hour, _tm->tm_min, _tm->tm_sec);
}

void signalControlAppHandler (int sig, siginfo_t *siginfo, void *data) {
    int spid = 0;

    if (siginfo->si_code <= 0) {
        spid = siginfo->si_pid;
    }

    switch (sig) {
    case SIGUSR2:
        printCurrentTimeToStdout ();
        fprintf (stdout, "Daemon executing now (PID = %d)\n", spid);
        break;
    default:
        printCurrentTimeToStdout ();
        fprintf (stdout, "Received unhandling signal [%d] (PID = %d)\n", sig, spid);
        break;
    }
}

int configureSignalControlAppHandlers () {

    sigemptyset(&sa.sa_mask);
    sa.sa_flags   = SA_SIGINFO;

    sa.sa_sigaction = signalControlAppHandler;

    sigaction (SIGUSR2,  &sa, 0); // Handle SIGHUP (SIG 1)

    return NO_ERROR;
}
