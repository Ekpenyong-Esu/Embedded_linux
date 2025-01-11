# AESD Socket Program

## Overview
The `aesdsocket` program is a socket-based application that listens for incoming connections on port 9000. It is designed to receive data, append it to a file located at `/var/tmp/aesdsocketdata`, and return the content to the client. The program also includes logging functionality to track connection events.

## Features
- Socket creation and binding to port 9000
- Listening for incoming client connections
- Logging messages to syslog
- Data reception and storage in `/var/tmp/aesdsocketdata`
- Graceful handling of signals for program termination

## Directory Structure
- `src/`: Contains the source code files.
  - `aesdsocket.c`: Main implementation of the socket-based program.
  - `queue.h`: Header file for queue management.
  - `logging.h`: Header file for logging functions.
- `Makefile`: Build instructions for compiling the program.
- `build.sh`: Script to automate the build process.
- `start-stop-daemon.sh`: Script to manage the daemon process.
- `README.md`: Documentation for the project.

## Build Instructions
To build the `aesdsocket` program, run the following command in the project root directory:

```bash
make
```

## Usage
To start the `aesdsocket` daemon, execute the following command:

```bash
./start-stop-daemon.sh start
```

To stop the daemon, use:

```bash
./start-stop-daemon.sh stop
```

## Dependencies
Ensure that the necessary development tools and libraries are installed for building the project.