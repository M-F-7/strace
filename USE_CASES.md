# ft_strace Use Cases

This file lists the use cases executed locally to validate `ft_strace` against the subject and to compare it with the real `strace` when useful.

## Subject-Oriented Checks

- Mandatory tracing without option works.
- Bonus `-c` works.
- PATH resolution works.
- Commands containing a slash are executed directly.
- Only one traced process is followed. Forked children are not traced, which matches `strace` without `-f`.
- The implementation now uses only the allowed ptrace requests:
  `PTRACE_SYSCALL`, `PTRACE_GETREGSET`, `PTRACE_SETOPTIONS`, `PTRACE_GETSIGINFO`, `PTRACE_SEIZE`, `PTRACE_INTERRUPT`, `PTRACE_LISTEN`.

## Executed Use Cases

1. `./ft_strace /bin/true`
Result: OK.
Check: starts on `execve(...)`, no internal sync `read/close` shown before the traced program.

2. `strace /bin/true`
Result: reference output collected.
Comparison: same global behavior and same syscall sequence family, but `ft_strace` keeps raw hexadecimal arguments and a lighter formatting.

3. `./ft_strace ls`
Result: OK.
Check: PATH lookup works and `ls` is executed without requiring `/bin/ls`.

4. `./ft_strace src/../test_bin`
Result: OK.
Check: a command containing `/` is treated as a direct path and not searched in PATH.

5. `env -i ./ft_strace ls`
Result: expected failure.
Observed message: `ft_strace: ls: No such file or directory`
Check: PATH bonus fails cleanly when PATH is absent.

6. `./ft_strace does-not-exist`
Result: expected failure.
Observed message: `ft_strace: does-not-exist: No such file or directory`

7. `./ft_strace Makefile`
Result: expected failure.
Check: a non-executable regular file is rejected.

8. `./ft_strace`
Result: expected failure.
Observed message: `usage: ft_strace [-c] command [args...]`

9. `./ft_strace -x /bin/true`
Result: expected failure.
Observed message: `ft_strace: unknown option: -x`

10. `./ft_strace -c`
Result: expected failure.
Observed message: `ft_strace: missing command`

11. `./ft_strace -c /bin/true`
Result: OK.
Check: summary table is printed and counts syscalls correctly enough for the bonus spirit.

13. `./ft_strace /bin/sh -c 'exit 42'`
Result: OK.
Check: final line reports `+++ exited with 42 +++`.

14. `./ft_strace /bin/sh -c 'kill -SEGV $$'`
Result: OK.
Check: signal is reported and the process ends with `+++ killed by SEGV +++`.

15. `./ft_strace /bin/sh -c 'true | true'`
Result: OK.
Check: no bogus `ENOSYS` spam anymore. The shell's own syscalls are traced, but forked children are not followed.

16. `strace /bin/sh -c 'true | true'`
Result: reference output collected.
Comparison: both trace the shell process only, report `SIGCHLD`, and do not follow the two `true` children without `-f`.

17. `./ft_strace /bin/sh -c 'sleep 0.1 & wait'`
Result: OK.
Check: background child creation no longer corrupts the tracer state.

18. `./ft_strace ./test_bin`
Result: OK.
Check: the test binary prints `salut` and `ft_strace` reports the surrounding syscalls correctly.

## Important Differences vs Real strace

- Formatting is intentionally simpler than real `strace`.
- Arguments are printed as raw hexadecimal values, not decoded structures/strings.
- Some special restart-related return codes may appear as generic unknown errors instead of the exact symbolic name used by real `strace`.
- This remains acceptable for the subject because the display only has to stay close to the original one, not identical.

## 32-bit Note

I checked the repository binaries with `file` and only 64-bit executables are available locally (`ft_strace`, `test_bin`, `a.out`).
No 32-bit executable or 32-bit runtime/toolchain is installed in this environment, so I could not run a real 32-bit execution test here.


opencode -s ses_1b9423944ffeCrjLeqXgdMSGVB