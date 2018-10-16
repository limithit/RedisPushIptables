/*
 *      Author: Gandalf
 *      email： zhibu1991@gmail.com
 */
#include "redismodule.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
int execute_fork()
{
        fflush(stdout);
        fflush(stderr);
        return fork();
}

int  execute_popen(pid_t *pid, const char *command)
{
        int             fd[2];

        if (-1 == pipe(fd))
                return -1;

        if (-1 == (*pid = execute_fork()))
        {
                close(fd[0]);
                close(fd[1]);
                return -1;
        }

        if (0 != *pid)  /* parent process */
        {
                close(fd[1]);
                return fd[0];
        }

        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        dup2(fd[1], STDERR_FILENO);
        close(fd[1]);
        if (-1 == setpgid(0, 0))
        {
                exit(EXIT_SUCCESS);
        }

        execl("/bin/sh", "sh", "-c", command, NULL);

        exit(EXIT_SUCCESS);
}

int IptablesPush_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
	if (argc != 3)
		return RedisModule_WrongArity(ctx);

	RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1],
	REDISMODULE_READ | REDISMODULE_WRITE);
	pid_t pid;
	int fd;
	char tmp_buf[4096];

	static char check_command[256], insert_command[256];
	sprintf(check_command, "iptables -C INPUT -s %s -j DROP",
			RedisModule_StringPtrLen(argv[1], NULL));
	sprintf(insert_command, "iptables -I INPUT -s %s -j DROP",
			RedisModule_StringPtrLen(argv[1], NULL));
	printf("%s || %s\n", RedisModule_StringPtrLen(argv[1], NULL),
			RedisModule_StringPtrLen(argv[2], NULL));
	fd = execute_popen(&pid, check_command);
	if (0 < read(fd, tmp_buf, sizeof(tmp_buf) - 1)) {
		close(fd);
		execute_popen(&pid, insert_command);
	}
        close(fd);
	RedisModule_StringSet(key, argv[2]);
	size_t newlen = RedisModule_ValueLength(key);
	RedisModule_CloseKey(key);
	RedisModule_ReplyWithLongLong(ctx, newlen);
	return REDISMODULE_OK;
}


/* This function must be present on each Redis module. It is used in order to
 * register the commands into the Redis server. */
int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (RedisModule_Init(ctx,"iptables-insert",1,REDISMODULE_APIVER_1)
        == REDISMODULE_ERR) return REDISMODULE_ERR;

    /* Log the list of parameters passing loading the module. */
    for (int j = 0; j < argc; j++) {
        const char *s = RedisModule_StringPtrLen(argv[j],NULL);
        printf("Module loaded with ARGV[%d] = %s\n", j, s);
    }

    if (RedisModule_CreateCommand(ctx,"iptables.push",
    		IptablesPush_RedisCommand,"write deny-oom",1,1,1) == REDISMODULE_ERR)
        return REDISMODULE_ERR;


    return REDISMODULE_OK;
}
