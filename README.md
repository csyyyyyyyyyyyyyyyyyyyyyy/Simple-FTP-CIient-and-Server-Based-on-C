## Documentation files
A simple FTP server & client written in C

    .
    ├── Makefile
    ├── src                    
    │   ├── myftp.h              
    │   ├── utils.c  
    │   ├── myftpdb.sqlite
    │   ├── server.c            
    │   ├── server.h
    │   ├── client.c 
    │   ├── client.h 
    │   ├── myftpc.c
    │   └── myftpd.c                
    └── README.md
    
## Description
- `Makefile`: A script file used to automate the compilation and installation process.
- `myftpc`: Executable file of FTP-client
- `myftpd`: Executable file of FTP-server
- `src`: Main directory for source code of the project.

### Details of the `src` Directory

- `myftp.h`: A common header file used across the FTP project, defining various structures and functions. 
- `utils.c`: Contains utility functions such as logging and error handling.
- `myftpdb.sqlite`: An SQLite database file used for storing data related to the FTP project.
- `server.c`: Contains the main functional code for the FTP server.
- `server.h`: A header file for the server, declaring functionalities of the FTP server.
- `client.c`: Contains the main functional code for the FTP client.
- `client.h`: A header file for the client, declaring functionalities of the FTP client.
- `myftpc.c`: Contains the startup and initialization logic for the FTP client.
- `myftpd.c`: Contains the startup and initialization logic for the FTP server.

## How to run

- Build
```
make build
```

- Server
    - When a server (myftpd) receives a request for a TCP connection request, it `forks()` and the child process accepts subsequent command messages from the client.
```
./myftpd src
```

- Client
    - Once a TCP connection is established, a client (myftpc) displays the prompt "myFTP%" and waits for user input.
```
./myftpc <IP>
```


## Commands

|command| args|description|
|:----|:----|:----|
|quit| – | quit the program|
|pwd| – |   print current directory|
|cd |path|  change current directory|
|dir| path|    list current directory|
|lpwd| – | print client current directory|
|lcd| path|    change client current directory|
|ldir| path| list client current directory|
|get| path1 path2|  get the file of 'path1' from server and save to 'path2'|
|put| path1 path2| put file of 'path1' to server and save to 'path2'|
|rm | path1 | remove the file in 'path1' from the server
|help| – |print help message|




