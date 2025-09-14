# Redis-cpp: A Redis Clone in C++

This project is a lightweight, high-performance, in-memory key-value store inspired by Redis, built from scratch in modern C++. It features a non-blocking, single-threaded server architecture and a custom hash table implementation with incremental rehashing to ensure low-latency operations.

## ✨ Features

- **Asynchronous Networking:** A single-threaded, event-driven server built using the poll() system call for high-concurrency I/O without the overhead of threads.
- **Efficient Data Structures:**
  - A custom-built **Hash Table** that uses incremental rehashing to avoid long pauses and latency spikes when the table needs to be resized.
  - A self-balancing **AVL Tree** to maintain sorted data and enable efficient rank-based queries.
- **Multiple Data Types:** Support for common Redis data types:
  - **Strings:** Basic key-value pairs.
  - **Sorted Sets (ZSETs):** A collection of unique members, each associated with a score, ordered by that score. Implemented with a hash table and an AVL tree for optimal performance.
- **Client-Server Architecture:** Includes both a server (`redis-server`) and a command-line client (`redis-cli`) for interaction.
- **Simple Binary Protocol:** A custom, lightweight, length-prefixed binary protocol for efficient communication between the client and server.
- **Core Redis Commands:** Implementation of several essential Redis commands (non-case sensitive), listed below.

## ⌨️ Supported Commands

### General

- `KEYS`: Returns all keys in the database.
- `DEL <key>`: Deletes a key.
- `PING [message]`: Checks server responsiveness.

### String

- `SET <key> <value>`: Sets the string value of a key.
- `GET <key>`: Gets the value of a key.

### Sorted Set (ZSET)

- `ZADD <key> <score> <member> [<score> <member> ...]`: Adds one or more members to a sorted set, or updates its score if it already exists.
- `ZREM <key> <member> [<member> ...]`: Removes one or more members from a sorted set.
- `ZRANGE <key> <start> <end>`: Returns the specified range of members in the sorted set, ordered from low to high scores.
- `ZREVRANGE <key> <start> <end>`: Returns the specified range of members, ordered from high to low scores.
- `ZSCORE <key> <member>`: Returns the score of a member in a sorted set.

## 🏗️ Project Structure

The repository is organized into the following directories:

``` bash
.
├── include/
│   ├── common/     # Shared utilities (Serialization, Deserialization)
│   ├── core/       # Core data structures (HashTable)
│   ├── net/        # Networking library (Server, Client, Network)
│   └── server/     # Redis application logic
├── src/
│   ├── common/
│   ├── core/
│   ├── net/
│   ├── server/
│   ├── redis-cli.cpp    # Client entry point
│   └── server-main.cpp  # Server entry point
├── bin/              # Compiled executables (created after build)
├── build/            # Object files (.o) (created after build)
└── Makefile          # Build script
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

#### String Commands

```bash
# SET: Store a key-value pair
$ ./bin/redis-cli SET myKey "Hello C++"
> SET myKey "Hello C++" 
(nil)

# GET: Retrieve the value for a key
$ ./bin/redis-cli GET myKey
> GET myKey 
"Hello C++"
```

#### Sorted Set Commands

```bash
# ZADD: Add members to a sorted set
$ ./bin/redis-cli ZADD leaderboard 100 alice 200 bob 150 charlie
> ZADD leaderboard 100 alice 200 bob 150 charlie
(integer) 3

# ZRANGE: Get members by rank (ordered by score)
$ ./bin/redis-cli ZRANGE leaderboard 0 -1
> ZRANGE leaderboard 0 -1
(arr) 3 elements:
 "alice"
 "charlie"
 "bob"

# ZSCORE: Get the score of a specific member
$ ./bin/redis-cli ZSCORE leaderboard charlie
> ZSCORE leaderboard charlie
"150.000000"

# ZREM: Remove a member
$ ./bin/redis-cli ZREM leaderboard alice
> ZREM leaderboard alice
(integer) 1

# Check the set again
$ ./bin/redis-cli ZRANGE leaderboard 0 -1
> ZRANGE leaderboard 0 -1
(arr) 2 elements:
 "charlie"
 "bob"
```

### Cleaning Up

To remove all compiled files (from `bin/` and `build/` directories), run:

``` bash
make clean
```

## 🏛️ Architecture

### Event Loop and Networking

The server operates on a single thread, using an event loop powered by `poll()`. This allows it to manage multiple client connections concurrently without blocking. All I/O operations are non-blocking, ensuring that the server remains responsive even under load. The core logic is contained within the `Server::run()` method.

### Data Storage

The in-memory data store is built on a primary `HashTable` that maps string keys to values. The values are stored in a `std::variant`, allowing each key to hold different data types, such as a simple string or a complex `SortedSet`.

- **Hash Table with Incremental Rehashing:** The primary key-value store. To handle resizing without causing performance degradation, it implements **incremental rehashing**. When the table's load factor exceeds a threshold, entries are migrated gradually from the old table to a new, larger one with every subsequent operation. This amortizes the cost of resizing, leading to smoother performance.
- **Sorted Set Implementation:** The `SortedSet` data type is implemented using a combination of two data structures for maximum efficiency:
  - A **Hash Table** maps members to their scores, providing $O(1)$ average time complexity for score lookups (`ZSCORE`).
  - A self-balancing **AVL Tree** stores members sorted by their scores, enabling efficient $O(log N)$ operations for adding, removing, and executing range queries (`ZADD`, `ZREM`, `ZRANGE`).

## 📄 License

This project is licensed under the **GNU General Public License v3.0**. A copy of the license is available in the `LICENSE` file in the repository.
