#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "io_operations.h"
#include "def.h"

char pidFilename[255];
char pidFullFilename[512];
const char *pidPath = "/var/run/";

int setPIDFilename (const char *daemonFilename) {

    size_t len, ext_len, path_len, i;
    const char *ext = ".pid";

    ext_len = strlen (ext);

    if (daemonFilename == NULL) return -1;

    len = strlen (daemonFilename);
    if (len + 1 > sizeof (pidFilename) - ext_len) len = sizeof (pidFilename) - 1 - ext_len;

    for (i = 0 ; i < len ; ++i) pidFilename[i] = daemonFilename[i];
    for (i = len ; i <= len + ext_len ; ++i) pidFilename[i] = ext[i - len];

    path_len = strlen (pidPath);
    if (path_len + 1 > sizeof (pidFullFilename)) path_len = sizeof (pidFullFilename) - 1;
    for (i = 0 ; i < path_len ; ++i) pidFullFilename[i] = pidPath[i];
    pidFullFilename[path_len] = 0;

    len = strlen (pidFilename);
    if (path_len + len + 1 > sizeof (pidFullFilename)) return -2;

    for (i = path_len ; i <= path_len + len ; ++i) pidFullFilename[i] = pidFilename[i - path_len];

    return NO_ERROR;
}

const char * getPIDFilename () {
    return pidFilename;
}

const char * getPIDFullFilename () {
    return pidFullFilename;
}

int readPIDFromFile (pid_t *pid) {
    *pid = 0;
    int erc;
    FILE *fl;

    fl = fopen (getPIDFullFilename (), "r");
    if (fl == NULL) return -1;

    erc = fscanf (fl, "%d", pid);
    fclose (fl);

    if (erc == 1) return NO_ERROR;
    return -2;
}

int savePIDToFile (pid_t pid) {
    FILE *fl;

    fl = fopen (getPIDFullFilename (), "w+");
    if (fl == NULL) return -1;

    fprintf (fl, "%d\n", pid);
    fclose (fl);

    return NO_ERROR;
}

int delPIDFile () {
    int erc;
    erc = unlink (getPIDFullFilename ());
    return erc;
}
