# CSE344 System Programming Homeworks

This repository contains all homework assignments and the final project completed for the **CSE344 System Programming** course at Gebze Technical University. The assignments focus on implementing various system programming concepts such as inter-process communication (IPC), multi-threading, synchronization, and network programming.

## Repository Structure
Each folder corresponds to a specific homework assignment or project:

- **cse344_hw1**: Introduction to system-level programming, focusing on concurrent file access and basic IPC mechanisms.
- **cse344_hw2**: Implementing a more advanced IPC mechanism using FIFOs, semaphores, and multi-threading for file management.
- **cse344_hw3**: A parking system simulation utilizing semaphores, threads, and synchronization techniques.
- **cse344_hw4**: A multi-threaded directory copying utility using worker-manager architecture with synchronization mechanisms.
- **cse344_hw5**: An enhanced version of HW4, incorporating condition variables and barriers for improved synchronization.
- **cse344_midterm_project**: A midterm project demonstrating comprehensive use of IPC, signal handling, and multi-threading concepts.
- **cse344_final_project**: A Pide Shop simulation implementing a real-world system using threads, synchronization, and network programming.

## Key Features
- **Multi-Threading**: Efficiently handles concurrent operations using thread pools.
- **Synchronization**: Utilizes mutexes, condition variables, and semaphores to manage shared resources.
- **Inter-Process Communication (IPC)**: Implements IPC techniques such as FIFOs, shared memory, and signals.
- **Network Programming**: Includes server-client architecture for communication and resource sharing.
- **Error Handling and Logging**: Ensures robust operation with proper error handling and detailed logging.

## Purpose
The assignments and projects aim to provide hands-on experience with system programming concepts and prepare students to design and implement efficient and reliable system-level applications.

## Usage
1. Navigate to the desired homework folder.
2. Use the provided Makefile to compile the project:
   ```bash
   make
