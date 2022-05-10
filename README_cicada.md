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