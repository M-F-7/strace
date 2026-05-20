#include "ft_strace.h"

/*
** Affiche le nom du syscall ou son numéro si inconnu
*/
static void print_syscall_name(long nr, int arch)
{
    const char *name;

    if (arch == ARCH_64)
        name = get_syscall_name_64(nr);
    else
        name = get_syscall_name_32(nr);

    if (name)
        dprintf(STDERR_FILENO, "%s", name);
    else
        dprintf(STDERR_FILENO, "syscall_%ld", nr);
}

static long long get_signed_arg(unsigned long long value, int arch)
{
    if (arch == ARCH_32)
        return ((long long)(int)value);
    return ((long long)value);
}

static int  print_access_mode(unsigned long long value)
{
    if (value == F_OK)
        return (dprintf(STDERR_FILENO, "F_OK"), 1);
    if (value == R_OK)
        return (dprintf(STDERR_FILENO, "R_OK"), 1);
    if (value == W_OK)
        return (dprintf(STDERR_FILENO, "W_OK"), 1);
    if (value == X_OK)
        return (dprintf(STDERR_FILENO, "X_OK"), 1);
    return (0);
}

static int  print_open_flags(unsigned long long value)
{
    int printed;

    printed = 0;
    if ((value & O_ACCMODE) == O_RDONLY)
        printed += dprintf(STDERR_FILENO, "O_RDONLY");
    else if ((value & O_ACCMODE) == O_WRONLY)
        printed += dprintf(STDERR_FILENO, "O_WRONLY");
    else if ((value & O_ACCMODE) == O_RDWR)
        printed += dprintf(STDERR_FILENO, "O_RDWR");
    if (value & O_CLOEXEC)
        printed += dprintf(STDERR_FILENO, "%sO_CLOEXEC", printed ? "|" : "");
    if (value & O_CREAT)
        printed += dprintf(STDERR_FILENO, "%sO_CREAT", printed ? "|" : "");
    if (value & O_TRUNC)
        printed += dprintf(STDERR_FILENO, "%sO_TRUNC", printed ? "|" : "");
    if (value & O_APPEND)
        printed += dprintf(STDERR_FILENO, "%sO_APPEND", printed ? "|" : "");
    return (printed > 0);
}

static int  print_mmap_prot(unsigned long long value)
{
    int printed;

    if (value == PROT_NONE)
        return (dprintf(STDERR_FILENO, "PROT_NONE"), 1);
    printed = 0;
    if (value & PROT_READ)
        printed += dprintf(STDERR_FILENO, "%sPROT_READ", printed ? "|" : "");
    if (value & PROT_WRITE)
        printed += dprintf(STDERR_FILENO, "%sPROT_WRITE", printed ? "|" : "");
    if (value & PROT_EXEC)
        printed += dprintf(STDERR_FILENO, "%sPROT_EXEC", printed ? "|" : "");
    return (printed > 0);
}

static int  print_mmap_flags(unsigned long long value)
{
    int printed;

    printed = 0;
    if (value & MAP_PRIVATE)
        printed += dprintf(STDERR_FILENO, "%sMAP_PRIVATE", printed ? "|" : "");
    if (value & MAP_SHARED)
        printed += dprintf(STDERR_FILENO, "%sMAP_SHARED", printed ? "|" : "");
    if (value & MAP_ANONYMOUS)
        printed += dprintf(STDERR_FILENO, "%sMAP_ANONYMOUS", printed ? "|" : "");
#ifdef MAP_DENYWRITE
    if (value & MAP_DENYWRITE)
        printed += dprintf(STDERR_FILENO, "%sMAP_DENYWRITE", printed ? "|" : "");
#endif
#ifdef MAP_FIXED
    if (value & MAP_FIXED)
        printed += dprintf(STDERR_FILENO, "%sMAP_FIXED", printed ? "|" : "");
#endif
    return (printed > 0);
}

static int  print_arch_prctl_code(unsigned long long value)
{
#ifdef ARCH_SET_FS
    if (value == ARCH_SET_FS)
        return (dprintf(STDERR_FILENO, "ARCH_SET_FS"), 1);
#endif
#ifdef ARCH_GET_FS
    if (value == ARCH_GET_FS)
        return (dprintf(STDERR_FILENO, "ARCH_GET_FS"), 1);
#endif
    return (0);
}

static int  print_rlimit_resource(unsigned long long value)
{
    if (value == RLIMIT_STACK)
        return (dprintf(STDERR_FILENO, "RLIMIT_STACK"), 1);
    if (value == RLIMIT_NOFILE)
        return (dprintf(STDERR_FILENO, "RLIMIT_NOFILE"), 1);
    if (value == RLIMIT_AS)
        return (dprintf(STDERR_FILENO, "RLIMIT_AS"), 1);
    return (0);
}

static int  print_special_arg(const char *name, int index,
    unsigned long long value)
{
    if (!name)
        return (0);
    if ((strcmp(name, "openat") == 0 || strcmp(name, "newfstatat") == 0
            || strcmp(name, "access") == 0) && index == 0
        && (int)value == AT_FDCWD)
        return (dprintf(STDERR_FILENO, "AT_FDCWD"), 1);
    if (strcmp(name, "access") == 0 && index == 1)
        return (print_access_mode(value));
    if (strcmp(name, "openat") == 0 && index == 2)
        return (print_open_flags(value));
    if (strcmp(name, "mmap") == 0 && index == 2)
        return (print_mmap_prot(value));
    if (strcmp(name, "mmap") == 0 && index == 3)
        return (print_mmap_flags(value));
    if (strcmp(name, "arch_prctl") == 0 && index == 0)
        return (print_arch_prctl_code(value));
    if (strcmp(name, "prlimit64") == 0 && index == 1)
        return (print_rlimit_resource(value));
    return (0);
}

static void print_default_arg(unsigned long long value, int arch)
{
    long long   signed_value;

    signed_value = get_signed_arg(value, arch);
    if (value == 0)
        dprintf(STDERR_FILENO, "NULL");
    else if ((signed_value < 0 && signed_value > -4096)
        || value <= 0x7fffffffULL)
        dprintf(STDERR_FILENO, "%lld", signed_value);
    else
        dprintf(STDERR_FILENO, "0x%llx", value);
}

static void print_syscall_arg(const char *name, int index,
    unsigned long long value, int arch)
{
    if (!print_special_arg(name, index, value))
        print_default_arg(value, arch);
}

/*
** Affiche les arguments du syscall (entrée)
** Format : syscall_name(arg0, arg1, ...) sans retour de ligne
*/
void    print_syscall_entry(t_regs *regs, int arch)
{
    int         i;
    const char  *name;

    if (arch == ARCH_64)
        name = get_syscall_name_64((long)regs->syscall_nr);
    else
        name = get_syscall_name_32((long)regs->syscall_nr);
    print_syscall_name((long)regs->syscall_nr, arch);
    dprintf(STDERR_FILENO, "(");
    for (i = 0; i < 6; i++)
    {
        if (i > 0)
            dprintf(STDERR_FILENO, ", ");
        print_syscall_arg(name, i, regs->args[i], arch);
    }
    dprintf(STDERR_FILENO, ")");
}

/*
** Affiche la valeur de retour du syscall (sortie)
** Détecte si c'est une erreur (retval proche de -1 en unsigned)
*/
void    print_syscall_exit(t_regs *regs, int arch)
{
    long long retval;

    (void)arch;
    retval = (long long)regs->retval;

    if (retval < 0 && retval >= -4095)
    {
        /* Erreur : affiche -ENAME (errno) */
        int err = (int)(-retval);
        dprintf(STDERR_FILENO, " = -1 %s (%s)\n",
            strerrorname_np(err) ? strerrorname_np(err) : "EUNKNOWN",
            strerror(err));
    }
    else
        dprintf(STDERR_FILENO, " = %lld\n", retval);
}

/*
** Affiche un signal reçu par le processus tracé
*/
void    print_signal(int sig, siginfo_t *si)
{
    const char *name;

    name = sigabbrev_np(sig);
    if (name)
        dprintf(STDERR_FILENO, "--- SIG%s {si_signo=SIG%s, si_code=%d",
            name, name, si->si_code);
    else
        dprintf(STDERR_FILENO, "--- signal %d {si_signo=%d, si_code=%d",
            sig, sig, si->si_code);

    /* Infos supplémentaires selon le signal */
    if (sig == SIGSEGV || sig == SIGBUS || sig == SIGFPE || sig == SIGILL)
        dprintf(STDERR_FILENO, ", si_addr=%p", si->si_addr);

    dprintf(STDERR_FILENO, "} ---\n");
}

/*
** Affiche le résumé des statistiques (-c)
** Format similaire à strace -c
*/
void    print_stats(t_tracer *tr)
{
    int     i;
    int     count;
    const t_syscall_stat *stats;

    dprintf(STDERR_FILENO,
        "%-6s %6s %11s %11s %s\n",
        "%", "calls", "errors", "syscall", "");
    dprintf(STDERR_FILENO,
        "------ ----------- ----------- -------------------------\n");

    if (tr->arch == ARCH_64)
    {
        stats = tr->stats64;
        count = MAX_SYSCALLS_64;
    }
    else
    {
        stats = tr->stats32;
        count = MAX_SYSCALLS_32;
    }

    for (i = 0; i < count; i++)
    {
        if (stats[i].count == 0)
            continue;
        dprintf(STDERR_FILENO,
            "%6.2f %11llu %11llu %s\n",
            tr->total_calls > 0
                ? (double)stats[i].count * 100.0 / (double)tr->total_calls
                : 0.0,
            stats[i].count,
            stats[i].errors,
            stats[i].name ? stats[i].name : "unknown");
    }

    dprintf(STDERR_FILENO,
        "------ ----------- ----------- -------------------------\n");
    dprintf(STDERR_FILENO,
        "%6s %11llu %11llu total\n",
        "100", tr->total_calls, tr->total_errors);
}
