#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <hiredis.h>
#include <syslog.h>
#include <fcntl.h>
#include <time.h>

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

int check_ipaddr (char *str)
{
        if (str == NULL || *str == '\0')
                return 1;

        struct sockaddr_in addr4;

        if (1 == inet_pton (AF_INET, str, &addr4.sin_addr))
                return 0;
        return 1;
}

int main(int argc, char **argv) {
	unsigned int j;
	redisContext *c;
	redisReply *reply;
	const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
	int port = (argc > 2) ? atoi(argv[2]) : 6379;

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
	daemon(0, 0);
	static char insert_command[256];
	static char msg[1024];
	pid_t pid;
	int fd;

    //reply = redisCommand(c,"psubscribe __key*__:*");
	reply = redisCommand(c, "psubscribe __key*__:expired");
	while (redisGetReply(c, (void *) &reply) == REDIS_OK) {
		printf("%s\n", reply->element[3]->str);
		
		if (!check_ipaddr(reply->element[3]->str)) {
		sprintf(insert_command, "iptables -D INPUT -s %s -j DROP",
				reply->element[3]->str);
		time_t t = time(NULL);
		struct tm *loc_time = localtime(&t);
		sprintf(msg, "pid=%d %02d/%02d-%02d:%02d:%02d iptables -D INPUT -s %s -j DROP\n",
				getpid(), loc_time->tm_mon+1, loc_time->tm_mday, loc_time->tm_hour, loc_time->tm_min, loc_time->tm_sec,
				reply->element[3]->str);
		write(logfd, msg, strlen(msg));
		fd = execute_popen(&pid, insert_command);
		close(fd);
	    }
		freeReplyObject(reply);
	}
	redisFree(c);
	close(logfd);
	return 0;
}
