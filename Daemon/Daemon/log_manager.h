#ifndef _LOG_MANAGER_H_
#define _LOG_MANAGER_H_

typedef enum {
    DEBUG    = 0,
    INFO     = 1,
    WARNING  = 2,
    ERROR    = 3,
    CRITICAL = 4
} LogMesPriority;

int setLogDaemonName (const char *daemon_name);
char * getDaemonName ();
void setMinMesPriority (LogMesPriority priority);
void logMessage (LogMesPriority priority, const char *message);

#endif // _LOG_MANAGER_H_
