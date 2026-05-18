NAME = ft_strace

SRCDIR = src
SRC =	main.c \
		args.c \
		trace.c \
		regs.c \
		print.c \
		stats.c \
		utils.c \
		syscall_table.c

OBJDIR = obj
OBJ = $(SRC:%.c=$(OBJDIR)/%.o)

INCLUDE = include

CC = cc
CFLAGS = -Wall -Wextra -Werror -D_GNU_SOURCE

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCLUDE) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

bonus: all

.PHONY: all clean fclean re bonus
