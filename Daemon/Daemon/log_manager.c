#include <string.h>
#include <syslog.h>
#include "log_manager.h"
#include "def.h"

char logDaemonName[64];
LogMesPriority minMesPriority;

int setLogDaemonName (const char *daemonName) {

    size_t len, i;

    if (daemonName == NULL) return -1;

    len = strlen (daemonName);
    if (len + 1 > sizeof (logDaemonName)) len = sizeof (logDaemonName) - 1;

    for (i = 0 ; i < len ; ++i) logDaemonName[i] = daemonName[i];

    logDaemonName[len] = 0;

    openlog (logDaemonName, LOG_PID, LOG_DAEMON);

    return NO_ERROR;
}

char * getDaemonName () {
    return logDaemonName;
}

void setMinMesPriority (LogMesPriority priority) {
    minMesPriority = priority;
}

int toStdPriority (LogMesPriority priority) {

    int stdLevel = LOG_EMERG;

    switch (priority) {
    case DEBUG:
        stdLevel = LOG_DEBUG;
        break;
    case INFO:
        stdLevel = LOG_INFO;
        break;
    case WARNING:
        stdLevel = LOG_WARNING;
        break;
    case ERROR:
        stdLevel = LOG_ERR;
        break;
    case CRITICAL:
        stdLevel = LOG_CRIT;
        break;
    default:
        stdLevel = LOG_EMERG;
        break;
    }

    return stdLevel;
}

void logMessage (LogMesPriority priority, const char *message) {

    if (minMesPriority <= priority)
        syslog (toStdPriority (priority), "%s", message);
}

