# RedisPushIptables

This README is just a fast quick start document.

In order to test the module you are developing, you can load the module using the following redis.conf configuration directive:

```
loadmodule /path/to/iptablespush.so
```

It is also possible to load a module at runtime using the following command:

```
MODULE LOAD /path/to/iptablespush.so
```

In order to list all loaded modules, use:

``
MODULE LIST
```
Finally, you can unload (and later reload if you wish) a module using the following command:
```

MODULE unload iptables-insert 
```
``````


# Redis must be run by root users, because iptables needs to submit the kernel.



```
127.0.0.1:6379> MODULE load /root/redis-4.0.9/src/modules/iptablespush.so
OK
```

```
127.0.0.1:6379> iptables.push 192.168.188.8 192.168.188.8
(integer) 13
```
