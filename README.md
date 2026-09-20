# Multithreaded C++ Web Server

A simple HTTP web server built from scratch in C++ to explore networking, concurrency, and operating system concepts through implementation.

## Features

- TCP socket-based HTTP server
- HTTP GET request handling
- Static file serving
- 404 Not Found responses
- 405 Method Not Allowed responses
- Basic path traversal protection
- MIME type detection
- Fixed-size thread pool
- Thread-safe request queue
- Multiple concurrent client connections

## Project Structure

    cpp-web-server/
    ├── server.cpp
    ├── www/
    │   └── index.html
    ├── .gitignore
    └── README.md

## Build

    g++ -std=c++20 -pthread server.cpp -o server

## Run

    ./server

The server listens on:

    http://localhost:8080

## Architecture

Incoming TCP connections are accepted by the main server thread and placed into a shared task queue.

A fixed number of worker threads retrieve connections from the queue and process HTTP requests.

        ┌── Worker 1
        ├── Worker 2
Client ──> Queue ───┼── Worker 3
        └── Worker 4

## HTTP Request Flow

    Client
       │
       ▼
    TCP Connection
       │
       ▼
    accept()
       │
       ▼
    Task Queue
       │
       ▼
    Worker Thread
       │
       ▼
    HTTP Request Parsing
       │
       ▼
    File Lookup
       │
       ├── File exists ──> 200 OK
       │
       └── File missing ─> 404 Not Found
       │
       ▼
    HTTP Response
       │
       ▼
    Client

## Topics Explored

This project was built to explore:

- Linux sockets
- TCP/IP
- HTTP
- C++ threads
- Mutexes
- Condition variables
- Thread pools
- Concurrency
- Operating system concepts
