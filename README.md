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
* [Point](#Point)
## Requirements

1. Redis4.0+
2. iptables `or pf or nftables`
3. gcc
4. make

## Features

RedisPushIptables is used to update firewall rules to reject the IP addresses for a specified amount of time or forever reject. however fail2ban relies on regular expressions. Once the application's log format has changed (the reason for the change may be due to version iteration), the filter needs to be reconfigured. RedisPushIptables does not have these concerns,it's used in the form of an API.

## Configuration

In order to test the module you are developing, you can load the module using the following redis.conf configuration directive:

```
loadmodule /etc/redis/modules/iptablespush.so
```

It is also possible to load a module at runtime using the following command:

```
MODULE LOAD /etc/redis/modules/iptablespush.so
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
```
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
  ```
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
root@debian:~/RedisPushIptables# /etc/init.d/ttl_iptables start
``` 

Logs are viewed in /var/log/ttl_iptables.log
```
root@debian:~# redis-cli TTL_DROP_INSERT 192.168.18.5 60  
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
* [accept_insert](#Core) - Filter table INPUT ADD ACCEPT
* [accept_delete](#Core) - Filter table INPUT DEL ACCEPT
* [drop_insert](#Core) - Filter table INPUT ADD DROP
* [drop_delete](#Core) - Filter table INPUT DEL DROP
* [ttl_drop_insert](#Core) - Dynamic delete filter table INPUT ADD DROP
```
127.0.0.1:6379>accept_insert 192.168.188.8
(integer) 13
127.0.0.1:6379>accept_delete 192.168.188.8
(integer) 13
127.0.0.1:6379>drop_delete 192.168.188.8
(integer) 13
127.0.0.1:6379>drop_insert 192.168.188.8
(integer) 13
127.0.0.1:6379>ttl_drop_insert 192.168.1.6 600 #600 seconds
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
#### Installing Packages on Linux

```
  #1: Compile hiredis
    cd redis-4.0**version**/deps/hiredis
    make 
    make install 
    echo /usr/local/lib >> /etc/ld.so.conf
    ldconfig 
    
  #2: git clone  https://github.com/limithit/RedisPushIptables.git
    cd RedisPushIptables
    make                      #OR make CFLAGS=-DWITH_IPSET    #OR make CFLAGS=-DWITH_NFTABLES
    make install
   ```
* If you need to enable ipset, you must configure the following settings
```
#ipset create block_ip hash:ip timeout 60 hashsize 4096 maxelem 10000000
#iptables -I INPUT -m set --match-set block_ip src -j DROP
#ipset create allow_ip hash:ip hashsize 4096 maxelem 10000000
#iptables -I INPUT -m set --match-set allow_ip src -j ACCEPT
```
The `timeout` parameter and  `ttl_drop_insert` parameter has the same effect. If the `timeout` parameter is configured, ipset is used to implement periodic deletion. If the `timeout` parameter is not configured, it is periodic deletion used `ttl_drop_insert`.

* If you need to enable nftables, you must configure the following settings 
```
#nft add table redis
#nft add chain redis INPUT \{ type filter hook input priority 0\; policy accept\; \}
```
#### Installing Packages on BSD and MacOS
```
  #1: Compile hiredis
    cd redis-4.0**version**/deps/hiredis
    make 
    make install 
    
  #2: git clone  https://github.com/limithit/RedisPushIptables.git
    cd RedisPushIptables
    make         
    make install
 ```

First edit the /etc/pf.conf file and add the code as follows:
```
table <block_ip> persist file "/etc/pf.block_ip.conf"
table <allow_ip> persist file "/etc/pf.allow_ip.conf"
block in log proto {tcp,udp,sctp,icmp} from <block_ip> to any
pass in proto {tcp,udp,sctp,icmp} from <allow_ip> to any
```
then 
```
touch /etc/pf.block_ip.conf
touch /etc/pf.allow_ip.conf
pfctl -F all -f /etc/pf.conf 
pfctl -e
```
BSD system does not provide a startup script

## HOWTOs
In theory, except for the C language native support API call, the corresponding library before the other language API calls must be re-encapsulated because the third-party modules are not supported by other languages. Only C, Python, Bash, Lua are shown here, and the principles of other languages are the same.

### C

C language only needs to compile and install hiredis. Proceed as follows:
```
root@debian:~/bookscode/redis-5.0.3/deps/hiredis#make install
```
cat examples.c

``` C
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

    reply = redisCommand(c,"drop_insert 192.168.18.3");
    printf("%d\n", reply->integer);
    freeReplyObject(reply);
    reply = redisCommand(c,"accept_insert 192.168.18.4");
    printf("%d\n", reply->integer);
    freeReplyObject(reply);

    reply = redisCommand(c,"drop_delete 192.168.18.3");
    printf("%d\n", reply->integer);
    freeReplyObject(reply);

    reply = redisCommand(c,"accept_delete 192.168.18.5");
    printf("%d\n", reply->integer);
    freeReplyObject(reply);
    
    reply = redisCommand(c,"ttl_drop_insert 192.168.18.5 600");
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

``` Python
       # COMMAND EXECUTION AND PROTOCOL PARSING
      def execute_command(self, *args, **options):
          "Execute a command and return a parsed response"
           .....
           .....
        
      def drop_insert(self, name):
          """
          Return the value at key ``name``, or None if the key doesn't exist
          """
          return self.execute_command('drop_insert', name)
      
      def accept_insert(self, name):
          """
          Return the value at key ``name``, or None if the key doesn't exist
          """
          return self.execute_command('accept_insert', name)
      
      def drop_delete(self, name):
          """
          Return the value at key ``name``, or None if the key doesn't exist
          """
          return self.execute_command('drop_delete', name)
  
      def accept_delete(self, name):
          """
          Return the value at key ``name``, or None if the key doesn't exist
          """
          return self.execute_command('accept_delete', name)
 
      def ttl_drop_insert(self, name, blocktime):
          """
          Return the value at key ``name``, or None if the key doesn't exist
          """
         return self.execute_command('ttl_drop_insert', name, blocktime)
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
``` Bash
#!/bin/bash
set -x
for ((i=1; i<=254; i++))
 do
redis-cli TTL_DROP_INSERT 192.168.17.$i 60 
done  
redis-cli DROP_INSERT 192.168.18.5 
redis-cli DROP_DELETE 192.168.18.5 
redis-cli ACCEPT_INSERT 192.168.18.5
redis-cli ACCEPT_DELETE 192.168.18.5
```
### Lua
```
git clone https://github.com/nrk/redis-lua.git
```
First edit the redis-lua/src/redis.lua file and add the code as follows:
``` Lua
redis.commands = {
    .....
    ttl              = command('TTL'),
    drop_insert      = command('drop_insert'),
    drop_delete      = command('drop_delete'),
    accept_insert    = command('accept_insert'),
    accept_delete    = command('accept_delete'),
    ttl_drop_insert  = command('ttl_drop_insert'),
    pttl             = command('PTTL'),         -- >= 2.6
     .....
```

simple.lua           `Luasocket don't forget to install`

``` Lua
package.path = "../src/?.lua;src/?.lua;" .. package.path
pcall(require, "luarocks.require")

local redis = require 'redis'

local params = {
    host = '127.0.0.1',
    port = 6379,
}

local client = redis.connect(params)
client:select(0) -- for testing purposes

client:drop_insert('192.168.1.1')
client:drop_delete('192.168.1.1')
client:ttl_drop_insert('192.168.1.2', '60')
local value = client:get('192.168.1.2')

print(value)
```
## Point

The master repository does not provide nftables repeat rule detection, but provides duplicate rule detection in the branch limithit-patch-1. Because this affects the execution speed of nftables, you need to make your own choices.

Lauchpad Pump Demo
=========================
~~Click below for a video of the pump controller demo in operation:~~

[![Pump Demo Video](https://github.com/limithit/RedisPushIptables/blob/master/demo.jpeg)](https://youtu.be/A9kL9ahC384)

Author Gandalf zhibu1991@gmail.com
