#include "ft_strace.h"

/*
** ft_strcmp : comparaison de deux chaînes
*/
int ft_strcmp(const char *a, const char *b)
{
    while (*a && *b && *a == *b)
    {
        a++;
        b++;
    }
    return ((unsigned char)*a - (unsigned char)*b);
}

/*
** ft_error : affiche un message d'erreur et exit(1)
*/
void    ft_error(const char *msg)
{
    dprintf(STDERR_FILENO, "ft_strace: %s\n", msg);
    exit(1);
}

/*
** ft_perror : affiche un message d'erreur avec errno et exit(1)
*/
void    ft_perror(const char *msg)
{
    dprintf(STDERR_FILENO, "ft_strace: %s: %s\n", msg, strerror(errno));
    exit(1);
}
