# Dropbox System 

In this project, your task is to implement a simplified version of Dropbox.

The server side should be capable of handling multiple clients simultaneously, functioning as a multi-threaded internet server. Upon establishing a connection with the server, the directories on both the server and client sides need to be synchronized. This means that any new file created, deleted, or updated on the server should reflect the same changes on the client side, and vice versa.

Unlike the official Dropbox service, your server should also maintain a logfile under the respective client's directory. This logfile should record the names and access times of created, deleted, and updated files. Additionally, it is important to handle SIGINT signal on both the server and client sides.

An example call for the server should be in the following format:

### BibakBOXServer [directory] [threadPoolSize] [portnumber]

where the directory is the server'sspecific area for file operations, threadPoolSize is the maximum number of threads active at a time, portnumberis the port server will wait for connection.

An example call from the client might be in the following format:

### BibakBOXClient [dirName] [portnumber]

where dirName is the name of the directory on server side and portnumber is the connection port of the serverNote that the client should return with a proper message when the server is,and server should prompt a message when a client connection is accepted (with the address of connection) to the screen.

<img src="Screenshots/part1.png>

<img src="Screenshots/part2.png>


# Test Cases

<img src="Screenshots/test1.png>

<img src="Screenshots/test2.png>

<img src="Screenshots/test3.png>

<img src="Screenshots/test4.png>

