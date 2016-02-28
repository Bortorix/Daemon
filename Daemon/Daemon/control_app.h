#ifndef _CONTROL_APP_H_
#define _CONTROL_APP_H_

//int startDaemon ();
//int restartDaemon ();
//int execDaemon ();
//int extractDaemonFilename (const char *path, const char **fname);
//int deletePIDFile (pid_t pid);

int mainControlAppLoop (short isExecuteDaemon);

int configureSignalControlAppHandlers ();

int startControlApp (const char *daemonName);

void printCurrentTimeToStdout ();

#endif // _CONTROL_APP_H_
