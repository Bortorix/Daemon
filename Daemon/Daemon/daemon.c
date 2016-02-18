#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include "daemon.h"
#include "io_operations.h"
#include "log_manager.h"
#include "def.h"
#include "status.h"

char log_buf[1024];
pid_t dpid = 0, chld_pid = 0, rpid = 0;

// SIGNALS
sigset_t sigset;
struct sigaction sa;
volatile short daemonStatus = CLEAR_STATUS;

volatile short isNeedSendAliveSignal = FALSE;

int extractDaemonFilename (const char *path, const char **fname) {

    *fname = strrchr(path, '/');

    if (*fname == NULL) return -1;

    *fname += sizeof(*path);

    return NO_ERROR;
}

int startDaemon (const char *daemonName) {

    int erc = NO_ERROR;

    setMinMesPriority (DEBUG);
    umask(~(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH));
    setsid();
    chdir("/");
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);

    if (IS_NOT_ERROR (erc)) {

        erc = setLogDaemonName (daemonName);

        switch (erc) {
        case -1:
            logMessage (ERROR, "Daemon name is NULL");
            break;
        default:
            logMessage (INFO, "Preliminary initialization finished successfully");
            break;
        }
    }
    return erc;
}

int restartDaemon () {

    int pid, erc = NO_ERROR;

    pid = fork ();

    switch(pid) {
    case 0:
        erc = execDaemon ();
        exit(erc);
    case -1:
        snprintf (log_buf, sizeof (log_buf), "Unable to create process by fork for restart. (%s)\n", strerror(errno));
        logMessage (ERROR, log_buf);
        break;
    default:
        snprintf (log_buf, sizeof (log_buf), "Daemon was restarted successfully (PID = %d)\n", pid);
        logMessage (ERROR, log_buf);
        break;
    }
    return erc;
}

int execDaemon () {
    int erc = NO_ERROR;
    pid_t old_dpid = 0;

    isNeedSendAliveSignal = FALSE;

    dpid = getpid ();
    rpid = 0;
    daemonStatus = CLEAR_STATUS;

    if (IS_NOT_ERROR (erc)) {
        erc = configureSignalHandlers ();
        if (HAS_ERROR (erc)) {
            snprintf (log_buf, sizeof (log_buf), "Ocurred error during configure signals. %s", strerror(errno));
            logMessage (ERROR, log_buf);
            return erc;
        }
    }

    if (IS_NOT_ERROR (erc)) {

        erc = setPIDFilename(getDaemonName ());

        switch (erc) {
        case -2:
            snprintf (log_buf, sizeof (log_buf), "The sum of string's length (%s) + (%s) too large",
                                                 getPIDFullFilename (), getPIDFilename ());
            logMessage (ERROR, log_buf);
            break;
        case -1:
            logMessage (ERROR, "Daemon name is NULL");
            break;
        default:
            snprintf (log_buf, sizeof (log_buf), "PID filename (%s) setup successfully", getPIDFullFilename ());
            logMessage (DEBUG, log_buf);
            break;
        }
    }

    if (IS_NOT_ERROR (erc)) {

        erc = readPIDFromFile (&old_dpid);

        switch (erc) {
        case -2:
            snprintf (log_buf, sizeof (log_buf), "Occured error during reading PID from the file (%s). %s",
                                                 getPIDFullFilename (), strerror(errno));
            logMessage (ERROR, log_buf);
            return erc;
        case -1:
            erc = NO_ERROR; // Ok, continue
            break;
        case 0:
            erc = kill (old_dpid, 0);
            if (erc == NO_ERROR) {
                snprintf (log_buf, sizeof (log_buf), "Instance of \'%s\' already executing with PID = %d", getDaemonName (), old_dpid);
                logMessage (CRITICAL, log_buf);
                return erc; // Daemon instance already executing
            } else if (errno == ESRCH) { // Try to remove *.pid file
                erc = deletePIDFile (old_dpid);
                if (erc == -1) return erc;
                else erc = NO_ERROR;
            } else {
                snprintf (log_buf, sizeof (log_buf), "Occured error trying to check existing previous instance of \'%s\'. %s", getDaemonName (), strerror(errno));
                logMessage (ERROR, log_buf);
                return errno;
            }
            break;
        default:
            snprintf (log_buf, sizeof (log_buf), "Unknown error code = %d returned by readPIDFromFile", erc);
            logMessage (ERROR, log_buf);
            erc = -2; // Unknown error
            return erc;
        }
    }

    if (IS_NOT_ERROR (erc)) {

        erc = savePIDToFile (dpid);

        switch (erc) {
        case -1:
            snprintf (log_buf, sizeof (log_buf), "Occured error during open the file (%s). %s", getPIDFullFilename (), strerror(errno));
            logMessage (ERROR, log_buf);
            break;
        default:
            snprintf (log_buf, sizeof (log_buf), "PID written to the file (%s) successfully", getPIDFullFilename ());
            logMessage (INFO, log_buf);
            break;
        }
    }

    if (IS_NOT_ERROR (erc)) logMessage (INFO, "Launch main loop");
    if (IS_NOT_ERROR (erc)) erc = mainloop();
    if (IS_NOT_ERROR (erc)) logMessage (INFO, "Main loop finished successfully");

    return erc;
}

int deletePIDFile (pid_t pid) {
    int erc;

    erc = delPIDFile ();

    switch (erc) {
    case -1:
        snprintf (log_buf, sizeof (log_buf), "Occured error during remove the file (%s) with PID = %d. %s", getPIDFullFilename (), pid, strerror(errno));
        logMessage (ERROR, log_buf);
        break;
    default:
        snprintf (log_buf, sizeof (log_buf), "The file (%s) with PID = %d was removed successfully", getPIDFullFilename (), pid);
        logMessage (DEBUG, log_buf);
        break;
    }
    return erc;
}

int mainloop () {

    int sec = 1, erc = NO_ERROR, maxSec = 10;
    struct timespec req, rem;

    req.tv_sec  = 1;
    req.tv_nsec = 0; //1000000000;

    while (daemonStatus == CLEAR_STATUS) {
        erc = nanosleep (&req, &rem); // erc = -1 when call interrupted by signal

        if (erc == -1) { // Interrupted by signal

            if (rem.tv_sec > 0 || rem.tv_nsec > 0) {
                req.tv_sec  = rem.tv_sec;
                req.tv_nsec = rem.tv_nsec;
                continue;
            }
        }

        if (sec <= maxSec && isNeedSendAliveSignal) kill(rpid, SIGUSR2); // While alive

        req.tv_sec  = 1;
        req.tv_nsec = 0;
        ++sec;
    }

    erc = deletePIDFile (dpid);
    if (daemonStatus == RESTART_STATUS) {
        erc = restartDaemon ();
    }

    return erc;
}

//void signalHandler (int sig) {
void signalCatching (int sig, siginfo_t *siginfo, void *data) {
    char sig_buf[64];
    //ucontext_t *context = (ucontext_t*) data;

    switch (sig) {
    case SIGHUP:   // 1. Exit app. Restart daemon                    /* Hangup (POSIX).  */
        daemonStatus = RESTART_STATUS;
        break;
    case SIGINT:   // 2. Stop (Ctrl+C). Is alive                     /* Interrupt (ANSI).  */
        if (siginfo->si_code <= 0) { // Generated by process
            rpid = siginfo->si_pid;
            isNeedSendAliveSignal = (isNeedSendAliveSignal == TRUE ? FALSE : TRUE);
        }
        break;
    case SIGQUIT:  // 3. Quit. Quit daemon                           /* Quit (POSIX).  */
        daemonStatus = FINISHED_STATUS;
        break;
    case SIGABRT:  // 6. Abort (Save to disk memory image).  /* Abort (ANSI).  */
        logMessage (DEBUG, "Received SIGABRT");
        break;
    case SIGSEGV:  // 11. Abort (Save to disk memory image). Ignored /* Segmentation violation (ANSI).  */
        logMessage (DEBUG, "Received SIGSEGV");
        break;
    case SIGTERM:  // 15. Exit. Quit daemon                          /* Termination (ANSI).  */
        daemonStatus = FINISHED_STATUS;
        break;
    case SIGCHLD:  // 17. Child status changed.                      /* Child status has changed (POSIX).  */ ???
        //Define later
        break;
    case SIGCONT:  // 18. Continue execution. Ignored                /* Continue (POSIX).  */
        logMessage (DEBUG, "Received SIGCONT");
        break;
    case SIGTSTP:  // 20. Freeze execution (Ctrl+Z). Ignored         /* Keyboard stop (POSIX).  */
        logMessage (DEBUG, "Received SIGTSTP");
        break;
    case SIGIO:    // 29. Receved data. Ignored
        logMessage (DEBUG, "Received SIGIO");
        break;
    default:
        snprintf (sig_buf, sizeof (sig_buf), "Received unhandling signal (%d)", sig);
        logMessage (DEBUG, sig_buf);
        break;
    }
}

//Signals

//#define	SIGHUP		1	/* Hangup (POSIX).  */
//#define	SIGINT		2	/* Interrupt (ANSI).  */
//#define	SIGQUIT		3	/* Quit (POSIX).  */
//#define	SIGILL		4	/* Illegal instruction (ANSI).  */
//#define	SIGTRAP		5	/* Trace trap (POSIX).  */
//#define	SIGABRT		6	/* Abort (ANSI).  */
//#define	SIGIOT		6	/* IOT trap (4.2 BSD).  */
//#define	SIGBUS		7	/* BUS error (4.2 BSD).  */
//#define	SIGFPE		8	/* Floating-point exception (ANSI).  */
//#define	SIGKILL		9	/* Kill, unblockable (POSIX).  */
//#define	SIGUSR1		10	/* User-defined signal 1 (POSIX).  */
//#define	SIGSEGV		11	/* Segmentation violation (ANSI).  */
//#define	SIGUSR2		12	/* User-defined signal 2 (POSIX).  */
//#define	SIGPIPE		13	/* Broken pipe (POSIX).  */
//#define	SIGALRM		14	/* Alarm clock (POSIX).  */
//#define	SIGTERM		15	/* Termination (ANSI).  */
//#define	SIGSTKFLT	16	/* Stack fault.  */
//#define	SIGCLD		SIGCHLD	/* Same as SIGCHLD (System V).  */
//#define	SIGCHLD		17	/* Child status has changed (POSIX).  */
//#define	SIGCONT		18	/* Continue (POSIX).  */
//#define	SIGSTOP		19	/* Stop, unblockable (POSIX).  */
//#define	SIGTSTP		20	/* Keyboard stop (POSIX).  */
//#define	SIGTTIN		21	/* Background read from tty (POSIX).  */
//#define	SIGTTOU		22	/* Background write to tty (POSIX).  */
//#define	SIGURG		23	/* Urgent condition on socket (4.2 BSD).  */
//#define	SIGXCPU		24	/* CPU limit exceeded (4.2 BSD).  */
//#define	SIGXFSZ		25	/* File size limit exceeded (4.2 BSD).  */
//#define	SIGVTALRM	26	/* Virtual alarm clock (4.2 BSD).  */
//#define	SIGPROF		27	/* Profiling alarm clock (4.2 BSD).  */
//#define	SIGWINCH	28	/* Window size change (4.3 BSD, Sun).  */
//#define	SIGPOLL		SIGIO	/* Pollable event occurred (System V).  */
//#define	SIGIO		29	/* I/O now possible (4.2 BSD).  */
//#define	SIGPWR		30	/* Power failure restart (System V).  */
//#define   SIGSYS		31	/* Bad system call.  */
//#define   SIGUNUSED	31

int configureSignalHandlers () {

    //siginfo_t siginfo;

    sigemptyset(&sigset);

    //sigaddset (&sigset, SIGILL);  // SIG 4
    sigaddset (&sigset, SIGSEGV); // SIG 11
    sigaddset (&sigset, SIGCONT); // SIG 18
    sigaddset (&sigset, SIGSTOP); // SIG 19
    sigaddset (&sigset, SIGTSTP); // SIG 20
    sigaddset (&sigset, SIGIO);   // SIG 29

    sigprocmask (SIG_BLOCK, &sigset, 0); //Block signals

    sigemptyset(&sa.sa_mask);
    sa.sa_flags     = SA_SIGINFO; // If cleared and the signal is caught, the signal-catching function will be entered as [void func(int signo);]

    //sa.sa_handler   = signalHandler;
    sa.sa_sigaction = signalCatching;

    sigaction (SIGHUP,  &sa, 0); // Handle SIGHUP (SIG 1)
    sigaction (SIGINT,  &sa, 0); // Handle SIGINT (SIG 2)
    sigaction (SIGQUIT, &sa, 0); // Handle SIGINT (SIG 3)
    sigaction (SIGABRT, &sa, 0); // Handle SIGABRT (SIG 6)
    sigaction (SIGTERM, &sa, 0); // Handle SIGTERM (SIG 15)
    sigaction (SIGCHLD, &sa, 0); // Handle SIGCHLD (SIG 17)

    return NO_ERROR;
}
