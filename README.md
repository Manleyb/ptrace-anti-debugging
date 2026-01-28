# Hardening CrackMe Binaries Against Reverse Engineering

This project demonstrates how to harden Linux binaries against dynamic analysis by taking the perspective of a defender. The included tool implements a protection that uses **Self-Modifying Code** and **Syscall Obfuscation** to defeat debuggers like GDB.

## Project Overview

The core tool (`ptrace-tracer.c`) exploits a fundamental property of Linux: a process can only have one active debugger (tracer) at a time. By having the program debug itself immediately upon launch, it occupies the single debugger slot.

If an external analyst tries to attach GDB to follow the child process, the OS blocks it because the child already has a tracer (its parent). The sensitive code path then fails silently.

## How the Protection Works

The protection uses a split-process architecture. The program forks into a parent and child. The child runs the actual program logic but is intentionally broken. The parent monitors the child via ptrace and fixes the broken code at runtime.

### Phase 1: Fork and Lock

The program splits into two processes. The child immediately requests to be traced by the parent, locking out other debuggers from attaching to it.

```c
pid_t pid = fork();

if (pid == 0) {
    ptrace(PTRACE_TRACEME, 0, NULL, 0);  // Child locks itself to parent
    child_logic();
} else {
    debugger_logic(pid);  // Parent becomes the shield
}
```

### Phase 2: The Trap (Syscall 10000)

If the password is correct, the child executes syscall 10000. This syscall does not exist in the Linux kernel. Under normal circumstances, the kernel would return an error. However, because the parent is monitoring via `PTRACE_SYSCALL`, the child pauses before the syscall executes.

### Phase 3: Runtime Patching

The parent catches the pause, reads the CPU registers, and performs a live patch:

1. Detects `regs.orig_rax == 10000` (the fake syscall)
2. Swaps `rdi` and `rsi` (arguments were intentionally passed in wrong order)
3. Rewrites `orig_rax` to `SYS_write` (the real syscall)

```c
if (regs.orig_rax == SC_OBFUSCATED) {
    regs.orig_rax = SYS_write;

    // Swap rdi (buffer) and rsi (fd) to match write(fd, buf, len)
    unsigned long long tmp = regs.rdi;
    regs.rdi = regs.rsi;
    regs.rsi = tmp;

    ptrace(PTRACE_SETREGS, pid, 0, &regs);
}
```

## Demonstration

**Normal execution:**
```
$ ./ptrace-tracer
IOLI Crackme Level 0x01
Password: 3214
Password OK :)
```

**Under GDB (following child):**
```
$ gdb ./ptrace-tracer
(gdb) set follow-fork-mode child
(gdb) run
IOLI Crackme Level 0x01
Password: 3214
[Program exits without printing success message]
```

When GDB follows the child, it attempts to become the tracer. But the child already called `PTRACE_TRACEME`, designating its parent as the tracer. The OS blocks GDB from also tracing the child. Without the parent's intervention, the invalid syscall 10000 is never fixed, and the success message is never printed.

## Building

```bash
# Standard build with debug symbols
make

# Stripped build (removes function names/symbols)
make stealth

# Clean build artifacts
make clean
```

## Limitations

**Static Analysis**: This technique only defeats dynamic analysis. Static analysis tools like Ghidra can examine the binary without executing it. The password constant (`0xc8e` = 3214), the fake syscall, and the parent's fixup logic are all visible in the disassembly.

**Performance**: The context switching overhead of `PTRACE_SYSCALL` monitoring reduces execution speed by approximately 60%.

**Brute Force**: The short password space (4 digits) remains vulnerable to scripting:

```bash
for i in $(seq 1 9999); do
    echo $i | ./ptrace-tracer | grep -q "OK" && echo "Password: $i"
done
```

## Additional Research

The accompanying `docs/Final Project Report.docx` analyzes further hardening techniques not implemented in this demo:

- **XOR String Encoding**: Hiding plaintext strings from the `strings` command
- **Decoy Branches**: Inserting fake success paths to confuse decompilers
- **Transform Functions**: Obscuring password values behind arithmetic operations
- **Compiler Hardening**: Utilizing `-fstack-protector-all`, `-fPIE`, and RELRO

## References

- [Complete Guide to Anti-Debugging in Linux (Part 2)](https://linuxsecurity.com/features/hacker-s-corner-complete-guide-to-anti-debugging-in-linux-part-2)
- [CodeBreakers 2006: Anti-Debugging Techniques](https://repo.zenk-security.com/Reversing%20.%20cracking/CodeBreakers%202006%20-%20AntiDebugging%20techniques.pdf)
