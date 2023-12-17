<div class="center">

# Virtual Memory Manager

## Overview

This project involves implementing a Virtual Memory Manager in C, employing the paging technique to simulate consecutive memory accesses. The objective is to translate logical addresses into 16-bit physical addresses within a 65536-byte virtual address space.

### Dependencies

The provided code is compatible with Clion and relies on *bison* and *flex* programs, available on Linux and MacOS.

### Project Scope

The project draws concepts from sections 8.5 (paging), 9.2 (demand paging), and 9.4 (page replacement). It simulates a physical memory comprising printable ASCII characters, featuring:
- 256 entries in the page table
- Page and frame size of 256 bytes
- 8 entries in the TLB
- 32 frames

## Functionality

The program reads and executes commands from standard input (*stdin*) for logical addresses. It translates these addresses to their corresponding physical addresses using either the Translation Look-aside Buffer (TLB) or the page table.

### Page Fault Handling

- On a TLB miss, the page is sought in the page table. If absent, a page fault occurs, triggering demand paging.
- Demand Paging: Upon page fault, a 256-byte page from the *BACKING_STORE.txt* file is read and stored in an available frame in physical memory.

## Command Format and Execution

The program handles read or write commands from *stdin* with the following formats:
- Write command: `w<logical-address>'<char-to-write>';` (e.g., `w20'b';`)
- Read command: `r<logical-address>;` (e.g., `r89;`)
