#include "ft_strace.h"

/*
** Détecte l'architecture du binaire tracé via l'en-tête ELF
** Lit /proc/PID/exe pour obtenir l'ELF header
** Retourne ARCH_64 ou ARCH_32
*/
static int  detect_arch(pid_t pid)
{
    char    path[64];
    int     fd;
    unsigned char ident[EI_NIDENT];
    int     ret;

    snprintf(path, sizeof(path), "/proc/%d/exe", (int)pid);
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return (ARCH_64); /* fallback */

    ret = read(fd, ident, EI_NIDENT);
    close(fd);

    if (ret < EI_NIDENT)
        return (ARCH_64);

    /* ELFCLASS32 = 1, ELFCLASS64 = 2 */
    if (ident[EI_CLASS] == ELFCLASS32)
        return (ARCH_32);
    return (ARCH_64);
}

/*
** Récupère les registres du processus via PTRACE_GETREGSET
** Abstrait la différence 32/64 bits dans t_regs
** Retourne 0 en succès, -1 en erreur
*/
int get_regs(pid_t pid, t_regs *regs, int *arch)
{
    struct iovec    iov;

    /* Détecte l'architecture si pas encore fait (arch == 0) */
    if (*arch == 0)
        *arch = detect_arch(pid);

    memset(regs, 0, sizeof(t_regs));
    regs->arch = *arch;

    if (*arch == ARCH_64)
    {
        struct user_regs_struct r64;

        iov.iov_base = &r64;
        iov.iov_len  = sizeof(r64);

        if (ptrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0)
            return (-1);

        regs->syscall_nr = r64.orig_rax;
        regs->retval     = r64.rax;
        regs->args[0]    = r64.rdi;
        regs->args[1]    = r64.rsi;
        regs->args[2]    = r64.rdx;
        regs->args[3]    = r64.r10;
        regs->args[4]    = r64.r8;
        regs->args[5]    = r64.r9;
    }
    else
    {
        /*
        ** Pour les binaires 32 bits, l'ABI i386 utilise
        ** struct user_regs_struct avec des champs 32 bits.
        ** Sur un kernel 64 bits, PTRACE_GETREGSET avec NT_PRSTATUS
        ** retourne une structure différente selon l'ABI du processus.
        */
        struct
        {
            uint32_t ebx, ecx, edx, esi, edi, ebp, eax;
            uint32_t ds, es, fs, gs;
            uint32_t orig_eax;
            uint32_t eip, cs, eflags, esp, ss;
        } r32;

        iov.iov_base = &r32;
        iov.iov_len  = sizeof(r32);

        if (ptrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) < 0)
            return (-1);

        regs->syscall_nr = r32.orig_eax;
        regs->retval     = r32.eax;
        regs->args[0]    = r32.ebx;
        regs->args[1]    = r32.ecx;
        regs->args[2]    = r32.edx;
        regs->args[3]    = r32.esi;
        regs->args[4]    = r32.edi;
        regs->args[5]    = r32.ebp;
    }
    return (0);
}
