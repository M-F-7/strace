#ifndef SYSCALL_TABLE_H
# define SYSCALL_TABLE_H

/*
** Tables des noms de syscalls Linux x86_64 et x86 (32-bit)
** Source : linux/arch/x86/entry/syscalls/
*/

/* Entrée d'une table de syscalls */
typedef struct s_syscall_entry
{
    long        number;
    const char  *name;
}   t_syscall_entry;

extern const t_syscall_entry g_syscalls_64[];
extern const t_syscall_entry g_syscalls_32[];
extern const int             g_syscalls_64_count;
extern const int             g_syscalls_32_count;

const char  *get_syscall_name_64(long nr);
const char  *get_syscall_name_32(long nr);

#endif
