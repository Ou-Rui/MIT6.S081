# MIT 6.S081 Lab3: Page Tables

## grade
```
== Test pte printout == 
$ make qemu-gdb
pte printout: OK (3.3s) 
== Test answers-pgtbl.txt == answers-pgtbl.txt: OK 
== Test count copyin == 
$ make qemu-gdb
count copyin: OK (0.8s) 
== Test usertests == 
$ make qemu-gdb
(132.3s) 
== Test   usertests: copyin == 
  usertests: copyin: OK 
== Test   usertests: copyinstr1 == 
  usertests: copyinstr1: OK 
== Test   usertests: copyinstr2 == 
  usertests: copyinstr2: OK 
== Test   usertests: copyinstr3 == 
  usertests: copyinstr3: OK 
== Test   usertests: sbrkmuch == 
  usertests: sbrkmuch: OK 
== Test   usertests: all tests == 
  usertests: all tests: OK 
== Test time == 
time: OK 
Score: 66/66
```
## 更新日志

### 2022.3.28
- complete `Simplify conpyin()/copyinstr()`
### 2022.3.26
- complete `Print a Page Table`
- complete `A Kernel Page Table per Process`