/*
 * 2019-03-14
 * Copyright (C)  西门吹雪
 * zhibu1991@gmail.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <hiredis.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <sys/file.h>

char *__progname;
static volatile sig_atomic_t got_sighup, got_sigchld;
#define _PATH_TTL_IPTABLES_PID	"/var/run/ttl_iptables.pid"

char *get_progname(char *argv0) {
	char *p, *q;
	p = __progname;
	if ((q = strdup(p)) == NULL) {
		perror("strdup");
		exit(1);
	}
	return q;
}

int redis_waitpid(pid_t pid) {
	int rc, status;
	do {
		if (-1 == (rc = waitpid(pid, &status, WUNTRACED))) {
			goto exit;
		}
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	exit:

	return rc;
}

int execute_fork() {
	fflush(stdout);
	fflush(stderr);
	return fork();
}

int execute_popen(pid_t *pid, const char *command) {
	int fd[2];

	if (-1 == pipe(fd))
		return -1;

	if (-1 == (*pid = execute_fork())) {
		close(fd[0]);
		close(fd[1]);
		return -1;
	}

	if (0 != *pid) /* parent process */
	{
		close(fd[1]);
		return fd[0];
	}

	close(fd[0]);
	dup2(fd[1], STDOUT_FILENO);
	dup2(fd[1], STDERR_FILENO);
	close(fd[1]);
	if (-1 == setpgid(0, 0)) {
		exit(EXIT_SUCCESS);
	}

	execl("/bin/sh", "sh", "-c", command, NULL);
	exit(EXIT_SUCCESS);
}

int check_ipaddr(char *str) {
	if (str == NULL || *str == '\0')
		return 1;

	struct sockaddr_in addr4;

	if (1 == inet_pton(AF_INET, str, &addr4.sin_addr))
		return 0;
	return 1;
}

static void sighup_handler(int x) {
	got_sighup = 1;
}

static void sigchld_handler(int x) {
	got_sigchld = 1;
}

static void quit(int x) {
	(void) unlink(_PATH_TTL_IPTABLES_PID);
	_exit(0);
}

static void sigchld_reaper(void) {
	int waiter;
	pid_t pid;

	do {
		pid = waitpid(-1, &waiter, WNOHANG);
		switch (pid) {
		case -1:
			if (errno == EINTR)
				break;
		case 0:
			break;
		default:
			break;
		}
	} while (pid > 0);
}

static void acquire_daemonlock(int closeflag) {
	static int fd = -1;
	char buf[3 * 100];
	const char *pidfile;
	char *ep;
	long otherpid;
	ssize_t num;

	if (closeflag) {
		/* close stashed fd for child so we don't leak it. */
		if (fd != -1) {
			close(fd);
			fd = -1;
		}
		return;
	}

	if (fd == -1) {
		pidfile = _PATH_TTL_IPTABLES_PID;
		/* Initial mode is 0600 to prevent flock() race/DoS. */
		if ((fd = open(pidfile, O_RDWR | O_CREAT, 0600)) == -1) {
			sprintf(buf, "can't open or create %s: %s", pidfile,
					strerror(errno));
			fprintf(stderr, "%s\n", buf);
			exit(1);
		}

		if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
			int save_errno = errno;

			bzero(buf, sizeof(buf));
			if ((num = read(fd, buf, sizeof(buf) - 1)) > 0&&
			(otherpid = strtol(buf, &ep, 10)) > 0 &&
			ep != buf && *ep == '\n' && otherpid != LONG_MAX) {
				sprintf(buf, "can't lock %s, otherpid may be %ld: %s", pidfile,
						otherpid, strerror(save_errno));
			} else {
				sprintf(buf, "can't lock %s, otherpid unknown: %s", pidfile,
						strerror(save_errno));
			}
			sprintf(buf, "can't lock %s, otherpid may be %ld: %s", pidfile,
					otherpid, strerror(save_errno));
			fprintf(stderr, "%s\n", buf);
			exit(1);
		}
		(void) fchmod(fd, 0644);
		(void) fcntl(fd, F_SETFD, 1);
	}

	sprintf(buf, "%ld\n", (long) getpid());
	(void) lseek(fd, (off_t) 0, SEEK_SET);
	num = write(fd, buf, strlen(buf));
	(void) ftruncate(fd, num);
}
int main(int argc, char **argv) {
	struct sigaction sact;
	bzero((char *) &sact, sizeof sact);
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_flags |= SA_RESTART;
	sact.sa_handler = sigchld_handler;
	(void) sigaction(SIGCHLD, &sact, NULL);
	sact.sa_handler = sighup_handler;
	(void) sigaction(SIGHUP, &sact, NULL);
	sact.sa_handler = quit;
	(void) sigaction(SIGINT, &sact, NULL);
	(void) sigaction(SIGTERM, &sact, NULL);
	acquire_daemonlock(0);

	redisContext *c;
	redisReply *reply;
	const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
	int port = (argc > 2) ? atoi(argv[2]) : 6379;
#ifdef BSD
	__progname = argv[0];
#else
	__progname = get_progname(argv[0]);
#endif
	int logfd;
	if ((logfd = open("/var/log/ttl_iptables.log", O_RDWR | O_CREAT | O_APPEND,
	S_IRUSR | S_IWUSR)) == -1) {
		fprintf(stderr, "can not open file:logfile\n");
	}

	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	c = redisConnectWithTimeout(hostname, port, timeout);
	if (c == NULL || c->err) {
		if (c) {
			fprintf(stderr, "Redis connection error: %s\n", c->errstr);
			redisFree(c);
		} else {
			fprintf(stderr, "Connection error: can't allocate redis context\n");
		}
		exit(1);
	}
#ifdef BSD
        pid_t pidt = fork();
	
        if (pidt != 0) {
                exit(0);
        }

        setsid();
        chdir("/");
     close(0); /* close stdin */
     close(1); /* close stdout */
     close(2); /* close stderr */
#else
        daemon(0, 0);
#endif
	acquire_daemonlock(0);
	static char insert_command[256];
	static char msg[1024];
	pid_t pid;
	int fd;

	//reply = redisCommand(c,"psubscribe __key*__:*");
	reply = redisCommand(c, "psubscribe __key*__:expired");
	while (redisGetReply(c, (void *) &reply) == REDIS_OK) {
		if (!check_ipaddr(reply->element[3]->str)) {
#ifdef WITH_IPSET
			sprintf(insert_command, "ipset del block_ip %s",
					reply->element[3]->str);
#elif BSD
			sprintf(insert_command, "pfctl -t block_ip -T del %s",
					reply->element[3]->str);
#elif WITH_NFTABLES
                        sprintf(insert_command, "nft delete rule redis INPUT `nft list table ip redis --handle --numeric |grep  -m1 \"ip saddr %s drop\"|grep -oe \"handle [0-9]*\"`",
					reply->element[3]->str);
#else
			sprintf(insert_command, "iptables -D INPUT -s %s -j DROP",
					reply->element[3]->str);
#endif
			time_t t = time(NULL);
			struct tm *loc_time = localtime(&t);
#ifdef WITH_IPSET
			sprintf(msg,
					"%04d/%02d/%02d %02d:%02d:%02d %s pid=%d ipset del block_ip %s\n",
					loc_time->tm_year + 1900, loc_time->tm_mon + 1, loc_time->tm_mday, loc_time->tm_hour,
					loc_time->tm_min, loc_time->tm_sec, __progname, getpid(),
					reply->element[3]->str);
#elif BSD
			sprintf(msg,
					"%04d/%02d/%02d %02d:%02d:%02d %s pid=%d pfctl -t block_ip -T del %s\n",
					loc_time->tm_year + 1900, loc_time->tm_mon + 1, loc_time->tm_mday, loc_time->tm_hour,
					loc_time->tm_min, loc_time->tm_sec, __progname, getpid(),
					reply->element[3]->str);
#elif WITH_NFTABLES
			sprintf(msg,
					"%04d/%02d/%02d %02d:%02d:%02d %s pid=%d nft delete rule redis INPUT `nft list table ip redis --handle --numeric |grep  -m1 \"ip saddr %s drop\"|grep -oe \"handle [0-9]*\"`\n",
					loc_time->tm_year + 1900, loc_time->tm_mon + 1, loc_time->tm_mday, loc_time->tm_hour,
					loc_time->tm_min, loc_time->tm_sec, __progname, getpid(),
					reply->element[3]->str);
#else
			sprintf(msg,
					"%04d/%02d/%02d %02d:%02d:%02d %s pid=%d iptables -D INPUT -s %s -j DROP\n",
					loc_time->tm_year + 1900, loc_time->tm_mon + 1, loc_time->tm_mday, loc_time->tm_hour,
					loc_time->tm_min, loc_time->tm_sec, __progname, getpid(),
					reply->element[3]->str);
#endif
			write(logfd, msg, strlen(msg));
			fd = execute_popen(&pid, insert_command);
			redis_waitpid(pid);
			close(fd);
		}
		freeReplyObject(reply);
		/* Check to see if we received a signal while running jobs. */
		if (got_sighup) {
			got_sighup = 0;
			redisFree(c);
			close(logfd);
		}
		if (got_sigchld) {
			got_sigchld = 0;
			sigchld_reaper();
		}
	}
	       time_t err_t = time(NULL);
               struct tm *err_loc_time = localtime(&err_t);
               static char err_msg[1024];
               sprintf(err_msg, "%04d/%02d/%02d %02d:%02d:%02d Redis connection error: %s\n",
                               err_loc_time->tm_year + 1900, err_loc_time->tm_mon + 1, err_loc_time->tm_mday,
		               err_loc_time->tm_hour, err_loc_time->tm_min, err_loc_time->tm_sec, c->errstr);
               write(logfd, err_msg, strlen(err_msg));
               close(logfd);
	
	return 0;
}

