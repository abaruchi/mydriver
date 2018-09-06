# mydriver
This repo contains a device driver to list open files of a given process inside /proc file system. 

# Installing

Before starting, make sure you have kernel headers and kernel source code installed in your Linux machine.

1. Compile it

```shell
$ make && make module
```

2. Load the kernel module

```shell
$ insmod mydriver.ko
```

3. Once the kernel module is loaded, you can check open files of a given process.

```shell
$ ./testDriverC /dev/mydriver0 3 <PID> <-PID
```

4. Device Driver will create a file inside /proc (`/proc/procfiles`) with all open files of a given process, including network sockets and pipes.


```shell
$ cat /proc/procfiles
/dev/null
/dev/null
/dev/null
socket:[7135]
/var/lock/zfs/zfs_lock
socket:[7137]
/var/lock/zfs/zfs_lock
pipe:[7141]
pipe:[7141]
``` 

*This device driver was developed during my Master Degree, back to 2010. Probably some kernel structures have changed since them.*

