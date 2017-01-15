#! /usr/bin/python

from string import Template
import subprocess

print "hello world"

template = Template(open('template.txt').read())

productVals = {'da':9,'db':36,'aa':9,'ab':9}

testOrder = 'da db aa ab'.split(' ')

values = range(1, 50)

startVals = {k:'' for k in testOrder}

def recurse(numAssigned, vals):
    if numAssigned >= len(testOrder):
        global numTests
        print 'End reached at {}... {}'.format(numTests, vals)
        return True
    k = testOrder[numAssigned]

    couldWork=False
    
    for val in values:
        vals[k]=val
        works = testBoards(vals)
        if works:
            reworks = recurse(numAssigned+1, vals)
            couldWork = couldWork or reworks
        else:
            continue
    return couldWork

def testBoards(vals={'a':5,'b':6}):
    valsA = vals
    valsB = dict()
    for k in testOrder:
        vala = valsA[k]
        if vala != '':
            if productVals[k] % valsA[k] != 0:
                return False
            valsB[k] = productVals[k]/valsA[k]
        else:
            valsB[k] = ''

    return testBoard(valsA) and testBoard(valsB)

    
numTests = 0

def testBoard(vals={'a':5,'b':6}, verbose=False):
    board = template.substitute(vals)
    board = board.replace(' \\ ','#') # Remove blank cases

    global numTests
    numTests += 1
    
    filename = 'cases/case{}.txt'.format(numTests)

    
    f=open(filename,'w')
    f.write(board)
    f.close()

    cmd="./kakuro {}".format(filename)
    
    if verbose:
        print cmd
    res=subprocess.call(cmd, shell=True)

    if res==0:
        # It worked!
        if verbose:
            print 'worked!'
        return True
    elif res==2:
        # No solutions
        
        if verbose:
            print 'No solutions'
        return False
    elif res==3:
        # Probably no solutions
        
        if verbose:
            print 'Probably no solutions'
        return False
