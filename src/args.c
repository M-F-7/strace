#include "ft_strace.h"

/*
** Résout le chemin d'une commande en cherchant dans PATH
** Retourne le chemin alloué (à libérer) ou NULL
*/
char    *resolve_path(const char *cmd)
{
    char    *path_env;
    char    *path_copy;
    char    *dir;
    char    *token;
    char    full[PATH_MAX];
    struct stat st;

    /* Toute commande contenant un slash doit etre executee telle quelle */
    if (strchr(cmd, '/'))
    {
        if (stat(cmd, &st) == 0 && S_ISREG(st.st_mode)
            && access(cmd, X_OK) == 0)
            return (strdup(cmd));
        return (NULL);
    }

    path_env = getenv("PATH");
    if (!path_env)
        return (NULL);

    path_copy = strdup(path_env);
    if (!path_copy)
        return (NULL);

    token = path_copy;
    while ((dir = strsep(&token, ":")) != NULL)
    {
        snprintf(full, PATH_MAX, "%s/%s", dir, cmd);
        if (stat(full, &st) == 0 && S_ISREG(st.st_mode)
            && access(full, X_OK) == 0)
        {
            free(path_copy);
            return (strdup(full));
        }
    }
    free(path_copy);
    return (NULL);
}

/*
** Usage et parsing des arguments
** Usage: ft_strace [-c] command [args...]
*/
int parse_args(int ac, char **av, t_tracer *tr)
{
    int i;

    i = 1;
    tr->opt_c = 0;
    tr->child_path = NULL;
    tr->child_argv = NULL;

    if (ac < 2)
    {
        dprintf(STDERR_FILENO,
            "usage: ft_strace [-c] command [args...]\n");
        return (-1);
    }

    /* Parse les options */
    while (i < ac && av[i][0] == '-')
    {
        if (ft_strcmp(av[i], "-c") == 0)
            tr->opt_c = 1;
        else
        {
            dprintf(STDERR_FILENO,
                "ft_strace: unknown option: %s\n", av[i]);
            return (-1);
        }
        i++;
    }

    if (i >= ac)
    {
        dprintf(STDERR_FILENO,
            "ft_strace: missing command\n");
        return (-1);
    }

    /* Résolution du chemin */
    tr->child_path = resolve_path(av[i]);
    if (!tr->child_path)
    {
        dprintf(STDERR_FILENO,
            "ft_strace: %s: No such file or directory\n", av[i]);
        return (-1);
    }

    /* argv du child : av[i..ac-1] */
    tr->child_argv = av + i;

    return (0);
}
