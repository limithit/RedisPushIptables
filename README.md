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
### Core
* [accept.insert](https://github.com/limithit/RedisPushIptables/blob/master/README.md) - Filter table INPUT ADD ACCEPT
* [accept.delete](https://github.com/limithit/RedisPushIptables/blob/master/README.md) - Filter table INPUT DEL ACCEPT
* [drop.insert](https://github.com/limithit/RedisPushIptables/blob/master/README.md) - Filter table INPUT ADD DROP
* [drop.delete](https://github.com/limithit/RedisPushIptables/blob/master/README.md) - Filter table INPUT DEL DROP
```
127.0.0.1:6379>accept.insert 192.168.188.8
(integer) 13
127.0.0.1:6379>accept.delete 192.168.188.8
(integer) 13
127.0.0.1:6379>drop.delete 192.168.188.8
(integer) 13
127.0.0.1:6379>drop.insert 192.168.188.8
(integer) 13
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
