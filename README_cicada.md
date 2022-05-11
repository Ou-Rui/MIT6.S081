# Lab8 locks

## 修改前
$ kalloctest
start test1
test1 results:
--- lock kmem/bcache stats
lock: kmem: #fetch-and-add 33030 #acquire() 433016
lock: bcache: #fetch-and-add 0 #acquire() 1242
--- top 5 contended locks:
lock: proc: #fetch-and-add 79893 #acquire() 514925
lock: proc: #fetch-and-add 42769 #acquire() 515222
lock: kmem: #fetch-and-add 33030 #acquire() 433016
lock: proc: #fetch-and-add 29077 #acquire() 515220
lock: virtio_disk: #fetch-and-add 12617 #acquire() 114
tot= 33030
test1 FAIL
start test2
total free number of pages: 32499 (out of 32768)
.....
test2 OK

## make grade
== Test running kalloctest == 
$ make qemu-gdb
(85.8s) 
== Test   kalloctest: test1 == 
  kalloctest: test1: OK 
== Test   kalloctest: test2 == 
  kalloctest: test2: OK 
== Test kalloctest: sbrkmuch == 
$ make qemu-gdb
kalloctest: sbrkmuch: OK (8.5s) 
== Test running bcachetest == 
$ make qemu-gdb
(6.6s) 
== Test   bcachetest: test0 == 
  bcachetest: test0: OK 
== Test   bcachetest: test1 == 
  bcachetest: test1: OK 
== Test usertests == 
$ make qemu-gdb
usertests: OK (101.8s) 
    (Old xv6.out.usertests failure log removed)
== Test time == 
time: OK 
Score: 70/70