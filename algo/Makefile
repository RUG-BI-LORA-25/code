.PHONY: run clean

CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC -shared

run: libalgo.so
	. ./.envrc && . ./.env/bin/activate && python3 wrapper.py

libalgo.so: libalgo.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f libalgo.so
