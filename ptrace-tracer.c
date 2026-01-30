#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/syscall.h>

// Arbitrary fake syscall number to intercept
#define SC_OBFUSCATED 10000

// Attempts to call the fake syscall.
// Arguments are intentionally passed in the wrong registers (msg in rdi, fd in rsi)
// to force the tracer to fix them up.
void secure_print(const char *str)
{                                                // A correct syscall would be write(int fd, const void *buf, size_t count);
    syscall(SC_OBFUSCATED, str, 1, strlen(str)); // where fd is an integer that the operating system uses to identify an open file,
}                                                //*buf is pointer to memory location and cont is number of bytes
                                                 // I switched *buff and fd which will cause the program to crash unless switched
void child_logic()
{
    if (ptrace(PTRACE_TRACEME, 0, NULL, 0) < 0) {
        perror("ptrace_traceme");
        exit(1);
    }

    // Signal parent that we are ready to be traced
    raise(SIGSTOP);

    int val;

    puts("IOLI Crackme Level 0x01");
    printf("Password: ");

    scanf("%d", &val);

    if (val == 0xc8e) {
        secure_print("Password OK :)\n");
    } else {
        puts("Invalid Password!");
    }
}

void debugger_logic(pid_t pid)
{
    int status;
    struct user_regs_struct regs;

    // Wait for child to stop at raise(SIGSTOP)
    waitpid(pid, &status, 0);

    // Kill child if tracer exits
    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);

    while (WIFSTOPPED(status)) {
        // Run to next syscall entry
        ptrace(PTRACE_SYSCALL, pid, 0, 0);
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) break;

        // Check register state on syscall entry
        ptrace(PTRACE_GETREGS, pid, 0, &regs);

        if (regs.orig_rax == SC_OBFUSCATED) {
            // Redirect to actual SYS_write
            regs.orig_rax = SYS_write;

            // Fixup args: swap rdi (buffer) and rsi (fd) to match write(fd, buf, len)
            unsigned long long tmp = regs.rdi;
            regs.rdi = regs.rsi;
            regs.rsi = tmp;

            ptrace(PTRACE_SETREGS, pid, 0, &regs);
        }

        // Run to syscall exit (ignore return value modification)
        ptrace(PTRACE_SYSCALL, pid, 0, 0);
        waitpid(pid, &status, 0);
    }
}

int main(int argc, char **argv)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        child_logic();
    } else {
        //parrent
        debugger_logic(pid);
    }

    return 0;
}
