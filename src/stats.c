#include "ft_strace.h"

/*
** Initialise les tables de statistiques
*/
void    init_stats(t_tracer *tr)
{
    int         i;
    const char  *name;

    memset(tr->stats64, 0, sizeof(tr->stats64));
    memset(tr->stats32, 0, sizeof(tr->stats32));
    tr->total_calls  = 0;
    tr->total_errors = 0;

    for (i = 0; i < MAX_SYSCALLS_64; i++)
    {
        name = get_syscall_name_64(i);
        tr->stats64[i].name = name;
    }
    for (i = 0; i < MAX_SYSCALLS_32; i++)
    {
        name = get_syscall_name_32(i);
        tr->stats32[i].name = name;
    }
}

/*
** Met à jour les statistiques après un syscall
** is_error : 1 si retval < 0, 0 sinon
*/
void    update_stats(t_tracer *tr, t_regs *regs, int is_error)
{
    unsigned long long  nr;
    t_syscall_stat      *stats;
    int                 max;

    nr = regs->syscall_nr;
    if (tr->arch == ARCH_64)
    {
        stats = tr->stats64;
        max   = MAX_SYSCALLS_64;
    }
    else
    {
        stats = tr->stats32;
        max   = MAX_SYSCALLS_32;
    }

    tr->total_calls++;
    if (is_error)
        tr->total_errors++;

    if (nr < (unsigned long long)max)
    {
        stats[nr].count++;
        if (is_error)
            stats[nr].errors++;
        if (!stats[nr].name)
        {
            if (tr->arch == ARCH_64)
                stats[nr].name = get_syscall_name_64((long)nr);
            else
                stats[nr].name = get_syscall_name_32((long)nr);
        }
    }
}
