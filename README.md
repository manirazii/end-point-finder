# Endpoint Scanner 🔍

A blazing fast multi-threaded web endpoint discovery tool written in C++.
## Features
- ⚡ 20+ concurrent threads for rapid scanning
- 📋 Built-in list of 100+ common endpoints
- 🛠️ Supports custom wordlists
- 📊 Detailed response metrics (status codes, timings, sizes)
- 🔄 Automatic redirect following (up to 5 levels)
- ⏱️ Configurable timeout (8 seconds default)

## Installation

### Prerequisites
- GCC/G++ (C++17 compatible)
- libcurl development libraries

```bash
# Ubuntu/Debian
sudo apt update && sudo apt install -y g++ libcurl4-openssl-dev

# CentOS/RHEL
sudo yum install -y gcc-c++ libcurl-devel
```
## Building
```bash
git clone https://github.com/yourusername/endpoint-scanner.git
cd endpoint-scanner
make
```
## Usage

```bash
# Basic scan
./scanner https://example.com

# With custom wordlist
./scanner https://example.com paths.txt

# Save results to file
./scanner https://example.com | tee results.txt
```

# Configuration
*Edit end-point-finder.cpp to change:*
``` cpp
#define TIMEOUT_SEC 8     // Request timeout
#define MAX_THREADS 20    // Concurrent threads
#define MAX_REDIRS 5      // Maximum redirects
```
## Troubleshooting
If you get linker errors:

```bash
# Ensure dependencies are installed
sudo apt install -y libcurl4-openssl-dev

# Compile manually if needed
g++ -std=c++17 end-point-finder.cpp -o scanner -lcurl -lpthread
```


