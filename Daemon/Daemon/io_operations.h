#ifndef _IO_OPERATIONS_H_
#define _IO_OPERATIONS_H_

int setPIDFilename (const char *daemonFilename);
const char * getPIDFilename ();
const char * getPIDFullFilename ();

int readPIDFromFile (pid_t *pid);
int savePIDToFile (pid_t pid);
int delPIDFile ();

#endif // _IO_OPERATIONS_H_
