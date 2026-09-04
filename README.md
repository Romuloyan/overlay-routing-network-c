# Distributed Overlay Routing Network in C

Academic networking project implementing a command-line overlay node in C. Each process can register in an overlay, establish TCP neighbour connections, exchange routing information and react to topology changes.

> This is a portfolio version of a two-person academic project. It preserves the original source code; course handouts, self-evaluation material and participant data are deliberately not included.

## What it demonstrates

- TCP sockets for peer-to-peer neighbour communication
- UDP sockets for interaction with a registration service
- `select()`-based I/O multiplexing across standard input, UDP, the TCP listening socket and active TCP neighbours
- Neighbour management and forwarding/routing-table maintenance
- A coordination mechanism for topology changes, including an additional safeguard for a cascading edge-failure case analysed during the project
- Modular C code split between command handling, connection management, routing and utility functions

## Build

Requirements: GCC and `make` on a Unix-like system.

```bash
make
```

The supplied Makefile builds the executable `OWR` with `-Wall` enabled.

## Run

```bash
./OWR <node-ip> <tcp-port> [registry-ip] [registry-port]
```

For example, this starts a local node listening on TCP port 58000:

```bash
./OWR 127.0.0.1 58000
```

An end-to-end overlay session requires a compatible registration service and multiple running nodes. Type `help` at the program prompt to view the supported commands.

## Source map

| File | Responsibility |
| --- | --- |
| `main.c` | Process setup and `select()` event loop |
| `cmd.c` | Command parsing and user-facing node operations |
| `connect.c` | TCP and UDP connection setup and message exchange |
| `routing.c` | Route processing and topology-change coordination |
| `utils.c` | Argument parsing, CLI dispatch and auxiliary functions |
| `structs.h`, `lib.h` | Shared data structures and constants |

## Verification performed for this portfolio version

- Clean GCC build completed successfully with `-Wall` enabled.
- A local node initialised successfully on `127.0.0.1:58000` and exited cleanly through its CLI.

The multi-node and registration-service scenarios are not claimed as part of this standalone audit because they depend on the course infrastructure and a separate node setup.

## Academic provenance and contribution

Developed for the **Redes de Computadores e Internet** course at Instituto Superior Técnico (2025/2026), as a two-person project. Rómulo Situ Antunes Yan contributed across the implementation and integration of the C program, including the networking, command and routing work. The original team source is retained; it is not presented as a solo project.

See [docs/ATTRIBUTION.md](docs/ATTRIBUTION.md) for the portfolio publication notes.
