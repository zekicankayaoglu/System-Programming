
# HOMEWORK 1
## 1. Write a program that takes up to three command-line arguments:

- $ appendMeMore filename num-bytes [x]

This file should open the specified filename (creating it if necessary) andappend num-bytes bytes to the file by using  write()  to write a byte at atime. By default, the program should open the file with the  O_APPENDflag,   but   if   a   third   command-line   argument   (x)   is   supplied,   then   theO_APPEND  flag   should   be   omitted,   and   instead   the   program   shouldperform an lseek(fd, 0, SEEK_END) call before each write(). Run two instancesof this program at the same time without the x argument to write 1 millionbytes to the same file:

- $ appendMeMore f1 1000000 & appendMeMore f1 1000000

Repeat the same steps, writing to a different file, but this time specifyingthe x argument:
- $ appendMeMore f2 1000000 x & appendMeMore f2 1000000 x
List the sizes of the files f1 and f2 using ls –l and explain the difference.

#
#
2.Implement  dup()  and  dup2()  using  fcntl()  and,   where  necessary,  close().(You   may     ignore   the   fact   that  dup2()  and  fcntl()  return   different  errnovalues for some error cases.) For dup2(), remember to handle the specialcase   where  oldfd  equals  newfd.   In   this   case,   you   should   check   whetheroldfd  is valid, which can be done by, for example, checking if  fcntl(oldfd,F_GETFL) succeeds. If oldfd is not valid, then the function should return –1with errno set to EBADF.
#
#
3. Write a program to verify that duplicated file descriptors share a fileoffset value and open file  