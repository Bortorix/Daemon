#ifndef _DAEMON_H_
#define _DAEMON_H_

int startDaemon (const char *daemonName);
int restartDaemon ();
int execDaemon ();
int extractDaemonFilename (const char *path, const char **fname);
int deletePIDFile (pid_t pid);
int mainloop ();

int configureSignalHandlers ();

#endif // _DAEMON_H_
