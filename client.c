#include <sys/types.h>

#include <sys/stat.h>

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "privatetcp.h"

extern char **environ;

void
privatetcp_client(struct privatetcp_client *client, const char *service)
{
	static const char client_path[] = CLIENT_PATH;
	static const char localip[] = "127.0.0.1";
	static const char localhost[] = "localhost";
	static const char *argv[2] = {".privatetcp", 0};
	static struct stat st;
	static char sportstr[8];
	static char cportstr[8];
	struct passwd *pw;

	snprintf(sportstr, sizeof(sportstr), "%u", client->sport);
	snprintf(cportstr, sizeof(cportstr), "%u", client->cport);
	umask(0077);
	errno = 0;
	pw = getpwuid(getuid());
	if (!pw && errno) {
		diesys("client: cannot get user info");
	}
	if (!pw) {
		die("client: user not found");
	}
	fprintf(stderr, "%s: client: user %lu(%s) group %lu on port %s(%s)\n",
	    progname, (unsigned long)getuid(), pw->pw_name,
	    (unsigned long)client->gid, sportstr, service);
	if (strstr(pw->pw_shell, "/nologin") ||
	    strstr(pw->pw_shell, "/false")) {
		die("client: user is not allowed to login");
	}
	if (chdir(pw->pw_dir) == -1) {
		diesys("client: cannot change to user's home directory");
	}
	if (!(environ = calloc(1, sizeof(*environ)))) {
		diesys("client: cannot clear environment");
	}
	if ((setenv("LOGNAME", pw->pw_name, 1) == -1) ||
	    (setenv("USER", pw->pw_name, 1) == -1) ||
	    (setenv("HOME", pw->pw_dir, 1) == -1) ||
	    (setenv("SHELL", pw->pw_shell, 1) == -1) ||
	    (setenv("PATH", client_path, 1) == -1) ||
	    (setenv("PORT", sportstr, 1) == -1) ||
	    (setenv("SERVICE", service, 1) == -1) ||
	    (setenv("PROTO", "TCP", 1) == -1) ||
	    (setenv("TCPLOCALIP", localip, 1) == -1) ||
	    (setenv("TCPLOCALPORT", sportstr, 1) == -1) ||
	    (setenv("TCPREMOTEIP", localip, 1) == -1) ||
	    (setenv("TCPREMOTEPORT", cportstr, 1) == -1) ||
	    (setenv("TCPLOCALHOST", localhost, 1) == -1) ||
	    (setenv("TCPREMOTEHOST", localhost, 1) == -1) ||
	    (setenv("TCPREMOTEINFO", pw->pw_name, 1) == -1)) {
		diesys("client: cannot set environment");
	}
	if (stat(".", &st) == -1) {
		diesys("client: cannot stat user home directory");
	}
	if (st.st_mode & 022) {
		die("client: refuse to run in group/world-writable home dir");
	}
	if (stat(argv[0], &st) == -1) {
		if (errno == ENOENT) {
			die("client: user does not have a script");
		}
		diesys("client: cannot stat user script");
	}
	if (st.st_mode & 022) {
		die("client: refuse to execute group/world-writable script");
	}
	if (st.st_mode & (S_ISUID | S_ISGID)) {
		die("client: refuse to execute setuid/setgid script");
	}
	execv(argv[0], (char *const *)argv);
	diesys("client: exec");
}
