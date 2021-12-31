# Makefile

CC=gcc
CFLAGS=-Wall -std=c11 -I.

# A #
A: A.c
	$(CC) $(CFLAGS) -o $@ $@.c -ljpeg

# A_p1 #
A_p1: A_p1.c
	$(CC) $(CFLAGS) -o $@ $@.c -ljpeg

# A_p2 #
A_p2: A_p2.c
	$(CC) $(CFLAGS) -o $@ $@.c -ljpeg

# A_p4 #
A_p4: A_p4.c
	$(CC) $(CFLAGS) -o $@ $@.c -ljpeg

# A_t1 #
A_t1: A_t1.c
	$(CC) $(CFLAGS) -pthread -o $@ $@.c -ljpeg

# A_t2 #
A_t2: A_t2.c
	$(CC) $(CFLAGS) -pthread -o $@ $@.c -ljpeg

# A_t4 #
A_t4: A_t4.c
	$(CC) $(CFLAGS) -pthread -o $@ $@.c -ljpeg


