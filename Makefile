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

tests:
	./$(NAME) ls -l 2> trace1.txt && strace ls -l 2> strace1.txt
	./$(NAME) /bin/true 2> trace2.txt && strace /bin/true 2> strace2.txt
	./$(NAME) src/../test_bin 2> trace3.txt && strace src/../test_bin 2> strace3.txt
	./$(NAME) -c ls -l 2> trace4.txt && strace -c ls -l 2> strace4.txt
	./$(NAME) -c ./test_bin 2> trace5.txt && strace -c ./test_bin 2> strace5.txt


clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re bonus
