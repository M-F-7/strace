#include "ft_strace.h"

/*
** environ est déclaré dans unistd.h sur Linux
** mais on le déclare explicitement pour être sûr
*/
extern char **environ;

int main(int ac, char **av)
{
    t_tracer    tr;

    memset(&tr, 0, sizeof(tr));

    if (parse_args(ac, av, &tr) < 0)
        return (1);

    return (run_tracer(&tr));
}
