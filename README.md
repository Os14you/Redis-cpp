# Redis-cpp: A Redis Clone in C++

This project is a lightweight, high-performance, in-memory key-value store inspired by Redis, built from scratch in modern C++. It features a non-blocking, single-threaded server architecture and a custom hash table implementation with incremental rehashing to ensure low-latency operations.

## ✨ Features

- **Asynchronous Networking:** A single-threaded, event-driven server built using the poll() system call for high-concurrency I/O without the overhead of threads.
- **Efficient Data Storage:** A custom-built hash table that uses incremental rehashing to avoid long pauses and latency spikes when the table needs to be resized.
- **Client-Server Architecture:** Includes both a server (redis-server) and a command-line client (redis-cli) for interaction.
- **Simple Binary Protocol:** A custom, lightweight, length-prefixed binary protocol for efficient communication between the client and server.
- **Core Redis Commands:** Implementation of several essential Redis commands (non-case sensitive):
  - `SET <key> <value>`
  - `GET <key>`
  - `DEL <key>`
  - `KEYS`
  - `PING [message]`

## 🏗️ Project Structure

The repository is organized into the following directories:

``` bash
.
├── include/      # Header files (.hpp) 
├── src/          # Source files (.cpp)
├── bin/          # Compiled executables (created after build)
├── build/        # Object files (.o) (created after build)
└── Makefile      # Build script
```

- `include/`: Contains all header files, defining the interfaces for the server, client, network layer, and data structures.
- `src/`: Contains the C++ source code implementing the logic defined in the headers.
- `bin/` and `build/`: These directories are created by the Makefile to store the final executables and intermediate object files, respectively.

## 🚀 Getting Started

Follow these instructions to build and run the project on a Linux-based system.

### Prerequisites

- A C++17 compliant compiler (e.g., g++)
- make

### Build

To compile the server and client executables, simply run the `make` command from the root of the project directory:

``` bash
make
```

This will compile all source files and place the `redis-server` and `redis-cli` executables in the `bin/` directory.

### Running the Server

Start the Redis server by running its executable. It will listen on the default Redis port `6379`.

``` bash
./bin/redis-server
```

You will see a confirmation message once the server is running:

``` txt
Server listening on port 6379 ...
```

### Using the Command-Line Client (CLI)

Open a new terminal and use the `redis-cli` to interact with the server. Here are some example commands:

#### SET: Store a key-value pair

``` bash
$ ./bin/redis-cli SET myKey "Hello C++"
> SET "myKey" "Hello C++" 
(nil)
```

#### GET: Retrieve the value for a key

``` bash
$ ./bin/redis-cli GET myKey
> GET "myKey" 
"Hello C++"
```

#### KEYS: List all keys in the store

``` bash
$ ./bin/redis-cli SET anotherKey 123
> SET "anotherKey" "123" 
(nil)

$ ./bin/redis-cli KEYS
> KEYS 
(arr) 2 elements:
 "anotherKey"
 "myKey"
```

#### DEL: Delete a key

``` bash
$ ./bin/redis-cli DEL myKey
> DEL "myKey" 
(integer) 1
```

#### PING: Check if the server is responsive

``` bash
$ ./bin/redis-cli PING
> PING 
"PONG"

$ ./bin/redis-cli PING "Is anyone there?"
> PING "Is anyone there?" 
"Is anyone there?"
```

### Cleaning Up

To remove all compiled files (from `bin/` and `build/` directories), run:

``` bash
make clean
```

## 🏛️ Architecture

### Event Loop and Networking

The server operates on a single thread, using an event loop powered by `poll()`. This allows it to manage multiple client connections concurrently without blocking. All I/O operations are non-blocking, ensuring that the server remains responsive even under load. The core logic is contained within the `Server::run()` method.

### Data Storage: Hash Table with Incremental Rehashing

The in-memory data is stored in a custom `HashTable` class. To handle resizing without causing performance degradation, the hash table implements **incremental rehashing**. When the table's load factor exceeds a certain threshold, a new, larger table is created. Instead of moving all entries at once (which would block the server), entries are migrated gradually from the old table to the new one with every subsequent `insert`, `lookup`, or `remove` operation. This amortizes the cost of resizing over many operations, leading to smoother performance.

## 📄 License

This project is licensed under the **GNU General Public License v3.0**. A copy of the license is available in the `LICENSE` file in the repository.
