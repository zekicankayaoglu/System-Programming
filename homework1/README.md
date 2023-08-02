
# HOMEWORK 1
#### 1. Write a program that takes up to three command-line arguments:

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
#
#
#

## HOW TO RUN PROGRAMS?
Firstly you must compile programs with ‘make’ then:

- You can run first part with this command:  ./appendMeMore filename byte x
- You can run second part with this command: ./duplicate
- You can run third part with this command: ./duplicate2

# Test Cases and Results
## Part 1:
You can see how to compile codes. Make compiles them and you can run first part as in that image.

<img src="screenshots/Picture1.png">

-In this first test, program writes 1000000 bytes to part1.txt and if I run it one more time, it writes 1000000 bytes more to this file. Finally there are 2m bytes in part1.txt

<img src="screenshots/Picture2.png">

-In this test there is not ‘x’ in command line so program writes bytes sequentially. First operation done writing to file then other operation starts to writing so at the end there are 2 million bytes becomes in file. There is no conflict.

<img src="screenshots/Picture3.png">

-In these tests, I use ‘x’ in command line and run same file name at the same time with 1000000 bytes. As you see there are 1000008 bytes but why not 2m ?
Because in x commands the program use ‘lseek’. And two writing operation try to do writing at the same time same place. So here both write operations may conflict and therefore the byte may not be written. So the byte number can be between 1 – 2 million.As you see in my test one of them 100008 and other is 1000000 bytes. So according to these test my program works corretly.

## Part 2:

<img src="screenshots/Picture4.png">

-In this test firstly, 3 file creating and then use dup and dup2 functions for them. Then prints fd values of these copy files and other files. And writes something to the copies. Let’s see the output now.

<img src="screenshots/Picture5.png">

-	Fd values: Dup: copy2(duplicated from fd3) ‘s value is different from fd3 because it is duplicated. It is 1 bigger than fd3. 
- Dup2: As you see fd1 was oldfd and fd2 is newfd so the copy fd is same as fd2 and different from fd1(oldfd). As we expected.
-	File contents: As you see program writes something to copy and copy2. 
- Dup: Copy was duplicate of fd3 so when program wrote to copy fd3 is updated and content came to here.
- Dup2: Copy2 was dup2(fd1,fd2) so when program wrote to copy2 fd1 is updated (because it is oldfd) and content came to here as you see in the terminal image.


## Part 3:

<img src="screenshots/Picture6.png">

-In this test firstly I create a file and I duplicate it as duplicated with dup function. First prints their offset values, then writes something to oldfd and then try to read two file to check are they same and then returns their getfl values to check are they same.
Let’s see the output results:

<img src="screenshots/Picture7.png">

-As you see, both offset values are 5 as we expected. After writing “hello again” to fd then we check offsets one more and they are same again.

-Then we check contents of files with read function. As you see both file have same contents.

-The last test is checking getfl values and they are same too for both files.

