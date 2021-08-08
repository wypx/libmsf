### 查看有哪些线程被调度到某个核
```sh
通过以下命令，可以看到有其他线程调度到被隔离的47核上。
[root@hn02-uhost-38-205 ~]# ps -eLo pid,user,lwp,psr,comm  | awk '{if($4==47) print $0}'
9 root         9  47 rcu_sched
291 root       291  47 cpuhp/47
292 root       292  47 watchdog/47
293 root       293  47 migration/47
294 root       294  47 ksoftirqd/47
296 root       296  47 kworker/47:0H
299 root       299  47 kdevtmpfs
4178 root      4178  47 kworker/47:1H
13260 root     13260  47 kworker/47:1
24231 root     24899  47 CPU 1/KVM
30799 root     30799  47 reactor_47
30799 root     30960  47 reactor_47
35040 root     35602  47 CPU 0/KVM
47102 root     47141  47 CPU 1/KVM
49031 root     49031  47 kworker/47:2
```