#! /usr/bin/python
from transforms import *

frontier = set([scrambledMessage])
explored = set()

def processValue(v, f):
    #frontier.remove(v)
    f.write(v+"\n")
    explored.add(v)
    for t in (pA, pB):
        new = apply_string(t, v)
        if new not in frontier and new not in explored:
            frontier.add(new)
    return

def generateAll():
    f = open('discovered.txt','w')
    while frontier:
        processValue(frontier.pop(), f)
        if len(explored)%1000 == 0:
            print "front: {}, explored: {}".format(len(frontier),len(explored))

    f.close()
    
def processValueHash(v, f):
    #frontier.remove(v)
    explored.add(hash(v))
    f.write(v+"\n")
    
    for t in (pA, pB):
        new = apply_string(t, v)
        newhash = hash(new)
        if new not in frontier and newhash not in explored:
            frontier.add(new)
    return

if __name__=="__main__":
    generateAll()
    
