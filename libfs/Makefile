lib := libfs.a

all: $(lib)

objects := fs.o disk.o

CC := gcc
CFlags := -Werror -Wall -Wextra -MMD -O
deps := $(patsubst %.o, %.d, $(objects))
-include $(deps)

libfs.a: $(objects)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFlags) -c -o $@ $<

clean: 
	rm -f $(lib) $(objects) $(deps)