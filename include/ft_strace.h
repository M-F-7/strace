#ifndef FT_STRACE_H
# define FT_STRACE_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <errno.h>
# include <signal.h>
# include <fcntl.h>
# include <limits.h>

# include <sys/ptrace.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/user.h>
# include <sys/uio.h>
# include <sys/stat.h>
# include <sys/mman.h>
# include <sys/resource.h>
# include <sys/signal.h>

# include <linux/ptrace.h>
# include <asm/prctl.h>
# include <elf.h>

# include "syscall_table.h"

/*
** Modes d'architecture
*/
# define ARCH_64  64
# define ARCH_32  32

/*
** Options ptrace
*/
# define PTRACE_OPTS (PTRACE_O_TRACESYSGOOD | PTRACE_O_EXITKILL)

/*
** Nombre max de syscalls connus
*/
# define MAX_SYSCALLS_64  400
# define MAX_SYSCALLS_32  400

/*
** Structure pour les statistiques (-c)
*/
typedef struct s_syscall_stat
{
    unsigned long long  count;
    unsigned long long  errors;
    const char          *name;
}   t_syscall_stat;

/*
** Contexte global du traceur
*/
typedef struct s_tracer
{
    pid_t               child_pid;
    int                 opt_c;          /* flag -c */
    int                 arch;           /* ARCH_64 ou ARCH_32 */
    int                 in_syscall;     /* toggle entry/exit */
    int                 trace_started;  /* ignorer la synchro avant execve */
    long                last_syscall;   /* dernier numéro de syscall */
    t_syscall_stat      stats64[MAX_SYSCALLS_64];
    t_syscall_stat      stats32[MAX_SYSCALLS_32];
    unsigned long long  total_calls;
    unsigned long long  total_errors;
    char                **child_argv;
    char                *child_path;
}   t_tracer;

/*
** Registres génériques (abstraction 32/64)
*/
typedef struct s_regs
{
    unsigned long long  syscall_nr;
    unsigned long long  retval;
    unsigned long long  args[6];
    int                 arch;
}   t_regs;

/* ---- prototypes ---- */

/* args.c */
int     parse_args(int ac, char **av, t_tracer *tr);
char    *resolve_path(const char *cmd);

/* trace.c */
int     run_tracer(t_tracer *tr);

/* regs.c */
int     get_regs(pid_t pid, t_regs *regs, int *arch);

/* print.c */
void    print_syscall_entry(t_regs *regs, int arch);
void    print_syscall_exit(t_regs *regs, int arch);
void    print_signal(int sig, siginfo_t *si);
void    print_stats(t_tracer *tr);

/* stats.c */
void    update_stats(t_tracer *tr, t_regs *regs, int is_error);
void    init_stats(t_tracer *tr);

/* utils.c */
void    ft_error(const char *msg);
void    ft_perror(const char *msg);
int     ft_strcmp(const char *a, const char *b);

#endif
