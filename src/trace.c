#include "ft_strace.h"

/*
** Variable globale pour signaler au traceur que le child est mort
*/
static volatile sig_atomic_t    g_child_exited = 0;
static pid_t                    g_child_pid    = 0;

/*
** Handler SIGINT/SIGTERM : interrompt le child proprement
*/
static void sighandler(int sig)
{
    (void)sig;
    if (g_child_pid > 0)
    {
        /*
        ** PTRACE_INTERRUPT : arrête le child sans lui envoyer de signal
        */
        ptrace(PTRACE_INTERRUPT, g_child_pid, NULL, NULL);
    }
    g_child_exited = 1;
}

/*
** Lance le processus child (appelé après fork, côté child)
** Lit sur le pipe pour attendre que le parent ait fait PTRACE_SEIZE,
** puis exec.
*/
static void child_exec(t_tracer *tr, int pipe_fd)
{
    char    buf;

    /* Attend le signal "go" du parent via le pipe */
    read(pipe_fd, &buf, 1);
    close(pipe_fd);

    execve(tr->child_path, tr->child_argv, environ);
    /* Si execve échoue */
    dprintf(STDERR_FILENO, "ft_strace: execve: %s\n", strerror(errno));
    _exit(1);
}

/*
** Traite un stop du processus tracé après PTRACE_SYSCALL
** Retourne 1 pour continuer, 0 pour stopper la boucle
*/
static int  handle_ptrace_stop(t_tracer *tr, pid_t pid, int status)
{
    int         sig;
    int         event;
    siginfo_t   si;
    t_regs      regs;
    int         is_error;

    sig   = 0;
    event = (status >> 16) & 0xff;

    /* Cas 1 : le processus a terminé normalement */
    if (WIFEXITED(status))
    {
        if (!tr->opt_c)
            dprintf(STDERR_FILENO, "+++ exited with %d +++\n",
                WEXITSTATUS(status));
        return (0);
    }

    /* Cas 2 : le processus a été tué par un signal */
    if (WIFSIGNALED(status))
    {
        if (!tr->opt_c)
            dprintf(STDERR_FILENO, "+++ killed by %s +++\n",
                sigabbrev_np(WTERMSIG(status))
                    ? sigabbrev_np(WTERMSIG(status)) : "unknown");
        return (0);
    }

    if (!WIFSTOPPED(status))
        return (1);

    sig = WSTOPSIG(status);

    /*
    ** Cas 3 : stop de syscall (SIGTRAP | 0x80)
    ** PTRACE_O_TRACESYSGOOD garantit le bit 0x80
    */
    if (sig == (SIGTRAP | 0x80))
    {
        if (get_regs(pid, &regs, &tr->arch) < 0)
        {
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            return (1);
        }

        if (!tr->in_syscall)
        {
            /* Entrée dans le syscall */
            tr->in_syscall   = 1;
            tr->last_syscall = (long)regs.syscall_nr;
            if (!tr->opt_c)
                print_syscall_entry(&regs, tr->arch);
        }
        else
        {
            /* Sortie du syscall */
            tr->in_syscall = 0;
            is_error = ((long long)regs.retval < 0 &&
                        (long long)regs.retval >= -4095);
            if (!tr->opt_c)
                print_syscall_exit(&regs, tr->arch);
            if (tr->opt_c)
                update_stats(tr, &regs, is_error);
        }

        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        return (1);
    }

    /*
    ** Cas 4 : SIGTRAP avec event (PTRACE_O_TRACE*)
    ** Couvre : PTRACE_EVENT_FORK, VFORK, CLONE, EXEC, EXIT
    */
    if (sig == SIGTRAP && event != 0)
    {
        /*
        ** PTRACE_EVENT_STOP : généré par PTRACE_INTERRUPT ou group-stop
        ** On utilise PTRACE_LISTEN pour les group-stops
        */
        if (event == PTRACE_EVENT_STOP)
        {
            if (ptrace(PTRACE_GETSIGINFO, pid, NULL, &si) < 0)
            {
                /* Group-stop : utiliser PTRACE_LISTEN */
                ptrace(PTRACE_LISTEN, pid, NULL, NULL);
                return (1);
            }
            /* PTRACE_INTERRUPT stop : relancer en mode syscall */
            if (g_child_exited)
            {
                ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
                return (1);
            }
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            return (1);
        }
        /* Autres événements : continuer */
        ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
        return (1);
    }

    /*
    ** Cas 5 : signal réel pour le child
    ** Récupère les infos via PTRACE_GETSIGINFO
    */
    if (ptrace(PTRACE_GETSIGINFO, pid, NULL, &si) == 0)
    {
        if (!tr->opt_c)
            print_signal(sig, &si);
    }

    /*
    ** Pour le SIGSTOP initial (après PTRACE_SEIZE + PTRACE_INTERRUPT),
    ** ne pas le délivrer au child
    */
    if (sig == SIGSTOP)
        sig = 0;

    /*
    ** Cas spécial : SIGSEGV, SIGBUS, SIGFPE, SIGILL
    ** Le child va mourir, on laisse le signal lui être délivré
    */
    ptrace(PTRACE_SYSCALL, pid, NULL, (void *)(long)sig);
    return (1);
}

/*
** Boucle principale du traceur
** Utilise PTRACE_SEIZE + PTRACE_SYSCALL
*/
int run_tracer(t_tracer *tr)
{
    pid_t           pid;
    pid_t           child;
    int             status;
    struct sigaction sa;
    int             pipefd[2];
    char            go;

    /* Pipe de synchronisation parent → child */
    if (pipe(pipefd) < 0)
        ft_perror("pipe");

    /* Installe les handlers de signal du traceur */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Fork */
    child = fork();
    if (child < 0)
        ft_perror("fork");

    if (child == 0)
    {
        /* Child : ferme l'extrémité d'écriture */
        close(pipefd[1]);
        child_exec(tr, pipefd[0]);
        /* Ne revient jamais */
    }

    /* Parent : ferme l'extrémité de lecture */
    close(pipefd[0]);

    g_child_pid   = child;
    tr->child_pid = child;

    /*
    ** PTRACE_SEIZE : attache au child sans l'arrêter
    ** Les options ptrace sont passées directement
    */
    if (ptrace(PTRACE_SEIZE, child, NULL, (void *)(long)PTRACE_OPTS) < 0)
    {
        close(pipefd[1]);
        ft_perror("PTRACE_SEIZE");
    }

    /*
    ** PTRACE_INTERRUPT : demande au child de s'arrêter
    ** afin qu'on puisse démarrer le traçage proprement
    */
    if (ptrace(PTRACE_INTERRUPT, child, NULL, NULL) < 0)
    {
        close(pipefd[1]);
        ft_perror("PTRACE_INTERRUPT");
    }

    /* Envoie le signal "go" au child pour qu'il commence son exec */
    go = 1;
    write(pipefd[1], &go, 1);
    close(pipefd[1]);

    /* Attend le stop causé par PTRACE_INTERRUPT (PTRACE_EVENT_STOP) */
    do {
        pid = waitpid(child, &status, 0);
    } while (pid < 0 && errno == EINTR);

    if (pid < 0)
        ft_perror("waitpid initial");

    /* Lance le traçage des syscalls */
    if (ptrace(PTRACE_SYSCALL, child, NULL, NULL) < 0)
        ft_perror("PTRACE_SYSCALL initial");

    /* Initialise les stats si -c */
    if (tr->opt_c)
        init_stats(tr);

    /* Boucle d'attente des événements */
    while (!g_child_exited)
    {
        pid = waitpid(-1, &status, __WALL);
        if (pid < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == ECHILD)
                break;
            ft_perror("waitpid loop");
        }

        if (!handle_ptrace_stop(tr, pid, status))
            break;
    }

    /* Affiche les stats si -c */
    if (tr->opt_c)
        print_stats(tr);

    return (0);
}
