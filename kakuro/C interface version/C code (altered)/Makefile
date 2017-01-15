# Copyright (C) 2009-2010 Adam Rosenfield
#
# This file is part of Kakuro, the Kakuro puzzle solver.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# gcc -pthread -o term term.c
CC=gcc
CFLAGS=-O3 -Wall -pthread

BINS = kakuro kakuro-blackjack kakuro-checkerboard kakuro-composite kakuro-duplicate kakuro-mistaken kakuro-nonconsecutive kakuro-product

PUZZLES = $(wildcard puzzles/*)

DOCS = README COPYING

all: $(BINS)

kakuro: kakuro.c
	$(CC) $(CFLAGS) kakuro.c -o $@

kakuro-blackjack: kakuro.c
	$(CC) $(CFLAGS) -DBLACKJACK_KAKURO kakuro.c -o $@

kakuro-checkerboard: kakuro.c
	$(CC) $(CFLAGS) -DCHECKERBOARD_KAKURO kakuro.c -o $@

kakuro-composite: kakuro.c
	$(CC) $(CFLAGS) -DCOMPOSITE_KAKURO kakuro.c -o $@

kakuro-duplicate: kakuro.c
	$(CC) $(CFLAGS) -DDUPLICATE_KAKURO kakuro.c -o $@

kakuro-mistaken: kakuro.c
	$(CC) $(CFLAGS) -DMISTAKEN_KAKURO kakuro.c -o $@

kakuro-nonconsecutive: kakuro.c
	$(CC) $(CFLAGS) -DNONCONSECUTIVE_KAKURO kakuro.c -o $@

kakuro-product: kakuro.c
	$(CC) $(CFLAGS) -DPRODUCT_KAKURO kakuro.c -o $@

.PHONY: clean dist
clean:
	rm -f $(BINS)

dist: kakuro.tar.gz
kakuro.tar.gz: kakuro.c Makefile $(PUZZLES) $(DOCS)
	tar czvf $@ $^
