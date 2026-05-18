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

/*
** Affiche les arguments du syscall (entrée)
** Format : syscall_name(arg0, arg1, ...) sans retour de ligne
*/
void    print_syscall_entry(t_regs *regs, int arch)
{
    int i;

    print_syscall_name((long)regs->syscall_nr, arch);
    dprintf(STDERR_FILENO, "(");
    for (i = 0; i < 6; i++)
    {
        if (i > 0)
            dprintf(STDERR_FILENO, ", ");
        dprintf(STDERR_FILENO, "0x%llx", regs->args[i]);
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
