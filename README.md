# TCP-Server-Client
## Introduction
This is a simple Socket Programming project that leverages C Language and certain libraries to create a basic TCP Client and a TCP Server with a functionality of transferring files.

## Features
- `UPLOAD`: Allows the user to upload a file to the server.
- `DOWNLOAD`: User can download files that are available in the server directory.
- `ECHO`: The user can send text messages to the server that gets echoed back to the client.
- `CLOSE`: This will terminate the connection and shutdown the server as well.

## Usage
`NOTE`: The client.c file and the server.c file should be present in different directories.
- The files that are to be uploaded should already be present in the same directory as the client.c file.
- The files that are to be download should similarly be present in the same directory as the server.c file.

Now we need to compile both the C files; Go into their respective directories and run each command accordingly:
```
gcc client.c -o client
gcc server.c -o server
```
This will create the binary files that can be executed now.
First head into the server directory and run:
```
./server
```
Then into the client directory to run the following:
```
./client
```
