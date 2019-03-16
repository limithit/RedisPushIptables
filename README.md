# RedisPushIptables

This README is just a fast quick start document. 
` Redis must be run by root users, because iptables needs to submit the kernel.`

content
================
* [Requirements](#requirements)
* [Features](#features)
* [Configuration](#Configuration)
* [Command](#Command)
* [HOWTOs](#HOWTOs)
* [Installation](#Installation)

## Requirements

1. Redis2.8+
2. iptables
3. gcc
4. make

## Features

RedisPushIptables is used to update firewall rules to reject the IP addresses for a specified amount of time or forever reject. however fail2ban relies on regular expressions. Once the application's log format has changed (the reason for the change may be due to version iteration), the filter needs to be reconfigured. RedisPushIptables does not have these concerns.

## Configuration

In order to test the module you are developing, you can load the module using the following redis.conf configuration directive:

```
loadmodule /path/to/iptablespush.so
```

It is also possible to load a module at runtime using the following command:

```
MODULE LOAD /path/to/iptablespush.so
```

In order to list all loaded modules, use:

```
MODULE LIST
```



Finally, you can unload (and later reload if you wish) a module using the following command:
```
MODULE unload iptables-input-filter
```


### Dynamic delete configuration

By default keyspace events notifications are disabled because while not very sensible the feature uses some CPU power. Notifications are enabled using the notify-keyspace-events of redis.conf or via the CONFIG SET. Setting the parameter to the empty string disables notifications. In order to enable the feature a non-empty string is used, composed of multiple characters, where every character has a special meaning according to the following table:

K     Keyspace events, published with __keyspace@<db>__ prefix.
E     Keyevent events, published with __keyevent@<db>__ prefix.
g     Generic commands (non-type specific) like DEL, EXPIRE, RENAME, ...
$     String commands
l     List commands
s     Set commands
h     Hash commands
z     Sorted set commands
x     Expired events (events generated every time a key expires)
e     Evicted events (events generated when a key is evicted for maxmemory)
A     Alias for g$lshzxe, so that the "AKE" string means all the events.
  
  At least K or E should be present in the string, otherwise no event will be delivered regardless of the rest of the string.For instance to enable just Key-space events for lists, the configuration parameter must be set to Kl, and so forth.The string KEA can be used to enable every possible event.

```
# redis-cli config set notify-keyspace-events Ex
```

You can load the module using the following redis.conf configuration directive:

```
notify-keyspace-events Ex
#notify-keyspace-events ""  # Comment out this line
```
Running ttl_iptables daemon with root user

```
root@debian:~/RedisPushIptables# ./ttl_iptables
``` 

Logs are viewed in /var/log/ttl_iptables.log
```
root@debian:~# redis-cli TTL.DROP.INSERT 192.168.18.5 60  
(integer) 12
root@debian:~# date
Fri Mar 15 09:38:49 CST 2019    
root@debian:~# iptables -L -n
Chain INPUT (policy ACCEPT)
target     prot opt source               destination         
DROP       all  --  192.168.18.5        0.0.0.0/0 
root@debian:~/RedisPushIptables# tail -f /var/log/ttl_iptables.log 
pid=5670 03/15-09:39:48 iptables -D INPUT -s 192.168.18.5 -j DROP
root@debian:~# iptables -L INPUT -n
Chain INPUT (policy ACCEPT)
target     prot opt source               destination
```

## Command
* [accept.insert](#Core) - Filter table INPUT ADD ACCEPT
* [accept.delete](#Core) - Filter table INPUT DEL ACCEPT
* [drop.insert](#Core) - Filter table INPUT ADD DROP
* [drop.delete](#Core) - Filter table INPUT DEL DROP
* [ttl.drop.insert](#Core) - Dynamic delete filter table INPUT ADD DROP
```
127.0.0.1:6379>accept.insert 192.168.188.8
(integer) 13
127.0.0.1:6379>accept.delete 192.168.188.8
(integer) 13
127.0.0.1:6379>drop.delete 192.168.188.8
(integer) 13
127.0.0.1:6379>drop.insert 192.168.188.8
(integer) 13
127.0.0.1:6379> TTL.DROP.INSERT 192.168.1.6 600 #600 seconds
(integer) 11
```
```
root@debian:~# iptables -L -n
Chain INPUT (policy ACCEPT)
target     prot opt source               destination         
DROP       all  --  192.168.188.8        0.0.0.0/0 
ACCEPT       all  --  192.168.188.8        0.0.0.0/0 
```
## Installation

```
  #1: Compile hiredis
    cd redis-4.0**version**/deps/hiredis
    make 
    make install 
    
  #2: git clone  https://github.com/limithit/RedisPushIptables.git
   cd RedisPushIptables
   make 
   ```
## HOWTOs
In theory, except for the C language native support API call, the corresponding library before the other language API calls must be re-encapsulated because the third-party modules are not supported by other languages. Here only demonstrate the similarities of C and Python in other languages.

### C

C language only needs to compile and install hiredis. Proceed as follows:
```
root@debian:~/bookscode/redis-5.0.3/deps/hiredis#make install
```
cat examples.c

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis.h>
int main(int argc, char **argv) {
    unsigned int j;
    redisContext *c;
    redisReply *reply;
    const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : 6379;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
}

    reply = redisCommand(c,"drop.insert 192.168.18.3");
    printf("%d\n", reply->integer);
    freeReplyObject(reply);
    reply = redisCommand(c,"accept.insert 192.168.18.4");
    printf("%d\n", reply->integer);
    freeReplyObject(reply);

    reply = redisCommand(c,"drop.delete 192.168.18.3");
    printf("%d\n", reply->integer);
    freeReplyObject(reply);

    reply = redisCommand(c,"accept.delete 192.168.18.5");
    printf("%d\n", reply->integer);
    freeReplyObject(reply);
    
    reply = redisCommand(c,"ttl.drop.insert 192.168.18.5 600");
    printf("%d\n", reply->integer);
    freeReplyObject(reply);
    redisFree(c);

    return 0;
}
```
gcc example.c -I/usr/local/include/hiredis –lhiredis

### Python

```
root@debian:~/bookscode# git clone https://github.com/andymccurdy/redis-py.git
```
After downloading, don't rush to compile and install. First edit the redis-py/redis/client.py file and add the code as follows:

```
       # COMMAND EXECUTION AND PROTOCOL PARSING
      def execute_command(self, *args, **options):
          "Execute a command and return a parsed response"
           .....
           .....
        
      def drop_insert(self, name):
          """
          Return the value at key ``name``, or None if the key doesn't exist
          """
          return self.execute_command('drop.insert', name)
      
      def accept_insert(self, name):
          """
          Return the value at key ``name``, or None if the key doesn't exist
          """
          return self.execute_command('accept.insert', name)
      
      def drop_delete(self, name):
          """
          Return the value at key ``name``, or None if the key doesn't exist
          """
          return self.execute_command('drop.delete', name)
  
      def accept_delete(self, name):
          """
          Return the value at key ``name``, or None if the key doesn't exist
          """
          return self.execute_command('accept.delete', name)
 
      def ttl_drop_insert(self, name, blocktime):
          """
          Return the value at key ``name``, or None if the key doesn't exist
          """
         return self.execute_command('ttl.drop.insert', name, blocktime)
```
```
root@debian:~/bookscode/redis-py# python setup.py build        
root@debian:~/bookscode/redis-py# python setup.py install        
root@debian:~/bookscode/8/redis-py# python
Python 2.7.3 (default, Nov 19 2017, 01:35:09) 
[GCC 4.7.2] on linux2
Type "help", "copyright", "credits" or "license" for more information.
>>> import redis
>>> r = redis.Redis(host='localhost', port=6379, db=0)
>>> r.drop_insert('192.168.18.7')
12L
>>> r.accept_insert('192.168.18.7')
12L
>>> r.accept_delete('192.168.18.7')
0L
>>> r.drop_delete('192.168.18.7')
0L
>>> r.ttl_drop_insert('192.168.18.7', 600)
12L
>>>
```
### Bash
```
#!/bin/bash
set -x
for ((i=1; i<=254; i++))
 do
redis-cli TTL.DROP.INSERT 192.168.17.$i 60 
done  
redis-cli DROP.INSERT 192.168.18.5 
redis-cli DROP.DELETE 192.168.18.5 
redis-cli ACCEPT.DELETE 192.168.18.5
redis-cli ACCEPT.INSERT 192.168.18.5
```

Lauchpad Pump Demo
=========================
Click below for a video of the pump controller demo in operation:

[![Pump Demo Video](https://github.com/limithit/RedisPushIptables/blob/master/demo.jpeg)](https://youtu.be/A9kL9ahC384)

Author Gandalf zhibu1991@gmail.com
