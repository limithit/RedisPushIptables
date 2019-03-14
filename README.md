# RedisPushIptables

This README is just a fast quick start document. 
Asynchronous implementation, will not block.
` Redis must be run by root users, because iptables needs to submit the kernel.`

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


## Dynamic deletion configuration

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
$ redis-cli config set notify-keyspace-events Ex
```

In order to test the module you are developing, you can load the module using the following redis.conf configuration directive:

```
notify-keyspace-events Ex
```
Running ttl_iptables daemon with root user

```
root@debian:~/RedisPushIptables# ./ttl_iptables
``` 

Logs are viewed in /var/log/ttl_iptables.log

### Core
* [accept.insert](https://github.com/limithit/RedisPushIptables/blob/master/README.md) - Filter table INPUT ADD ACCEPT
* [accept.delete](https://github.com/limithit/RedisPushIptables/blob/master/README.md) - Filter table INPUT DEL ACCEPT
* [drop.insert](https://github.com/limithit/RedisPushIptables/blob/master/README.md) - Filter table INPUT ADD DROP
* [drop.delete](https://github.com/limithit/RedisPushIptables/blob/master/README.md) - Filter table INPUT DEL DROP
* [ttl.accept.insert](https://github.com/limithit/RedisPushIptables/blob/master/README.md) - Dynamic deletion filter table INPUT ADD ACCEPT
* [ttl.drop.insert](https://github.com/limithit/RedisPushIptables/blob/master/README.md) - Dynamic deletion filter table INPUT ADD DROP
```
127.0.0.1:6379>accept.insert 192.168.188.8
(integer) 13
127.0.0.1:6379>accept.delete 192.168.188.8
(integer) 13
127.0.0.1:6379>drop.delete 192.168.188.8
(integer) 13
127.0.0.1:6379>drop.insert 192.168.188.8
(integer) 13
127.0.0.1:6379> TTL.ACCEPT.INSERT 192.168.1.5 60
(integer) 11
127.0.0.1:6379> TTL.DROP.INSERT 192.168.1.6 600
(integer) 11
```
```
root@debian:~# iptables -L -n
Chain INPUT (policy ACCEPT)
target     prot opt source               destination         
DROP       all  --  192.168.188.8        0.0.0.0/0 
ACCEPT       all  --  192.168.188.8        0.0.0.0/0 
```
Lauchpad Pump Demo
=========================
Click below for a video of the pump controller demo in operation:

[![Pump Demo Video](https://github.com/limithit/RedisPushIptables/blob/master/demo.jpeg)](https://youtu.be/A9kL9ahC384)

Author Gandalf zhibu1991@gmail.com
