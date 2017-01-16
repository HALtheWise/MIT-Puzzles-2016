#!/usr/bin/python

strings = []
matches = []

def readAllToList(fn='discovered.txt'):
    global strings
    strings = []
    with open(fn) as f:
        strings=[s.strip() for s in f.readlines()]

def hasNoTwoSpaces(s):
    return '  ' not in s

def readThroughFilter(fn='discovered.txt'):
    global matches
    matches = []
    with open(fn) as f:
        i=0
        while True:
            i += 1
            s = f.readline().strip()
            if not s:
                break
            if hasNoTwoSpaces(s):
                matches.append(s)

            if i % 1000 == 0:
                print i
