# high-performance-load-balancer
High-performance Layer 4 TCP load balancer implemented in C++ using epoll-based non-blocking I/O, supporting round-robin and consistent hashing strategies with active backend health monitoring.
Overview

This project implements a high-performance Layer 4 (TCP) load balancer in C++ designed for scalability and low-latency request forwarding. The system uses epoll-based non-blocking I/O to efficiently handle thousands of concurrent connections and supports multiple load balancing strategies.

Key Features

Event-driven architecture using epoll (Linux)

Non-blocking socket I/O

Round-robin and consistent hashing algorithms

Active health monitoring of backend servers

Connection mapping and bidirectional TCP forwarding

Modular architecture for extensibility

Designed for performance benchmarking and scalability testing

System Design Goals

Minimize context switching

Optimize throughput under high concurrency

Ensure fault tolerance through backend health checks

Provide clean separation between networking, scheduling, and monitoring layers

Technologies Used

C++17

POSIX sockets

epoll (Linux I/O multiplexing)

STL containers and modern C++ constructs
