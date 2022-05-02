# Lab7 Multi-Threading

## make grade
== Test uthread == 
$ make qemu-gdb
uthread: OK (3.8s) 
== Test answers-thread.txt == answers-thread.txt: OK 
== Test ph_safe == make[1]: 进入目录“/home/cicada/Documents/visual-studio-code-project/xv6-labs-2020”
gcc -o ph -g -O2 notxv6/ph.c -pthread
make[1]: 离开目录“/home/cicada/Documents/visual-studio-code-project/xv6-labs-2020”
ph_safe: OK (12.6s) 
== Test ph_fast == make[1]: 进入目录“/home/cicada/Documents/visual-studio-code-project/xv6-labs-2020”
make[1]: “ph”已是最新。
make[1]: 离开目录“/home/cicada/Documents/visual-studio-code-project/xv6-labs-2020”
ph_fast: OK (28.6s) 
== Test barrier == make[1]: 进入目录“/home/cicada/Documents/visual-studio-code-project/xv6-labs-2020”
gcc -o barrier -g -O2 notxv6/barrier.c -pthread
make[1]: 离开目录“/home/cicada/Documents/visual-studio-code-project/xv6-labs-2020”
barrier: OK (11.4s) 
== Test time == 
time: OK 
Score: 60/60