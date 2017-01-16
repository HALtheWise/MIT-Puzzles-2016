#! /usr/bin/python

from string import Template
import subprocess

print "hello world"

template = Template(open('template.txt').read())

productVals = {'aa':28, 'da':25, 'ab':280, 'db':406, 'ac':506, 'dc':1023, 'ad':992, 'dd':870, 'ae':88, 'de':84,
             'af':68, 'df':1640, 'ag':63, 'dg':12, 'ah':437, 'dh':348, 'ai':266, 'di':209, 'aj':114, 'dj':136,
             'ak':588, 'dk':810, 'al':18, 'dl':88, 'am':196, 'dm':190, 'an':256, 'dn':1054, 'ao':595, 'do':110,
             'ap':1073, 'dp':208, 'aq':220, 'ar':144}

testOrder = ['aa', 'da', 'ab', 'db', 'ac', 'dc', 'ad', 'dd', 'ae', 'de',
             'af', 'df', 'ag', 'dg', 'ah', 'dh', 'ai', 'di', 'aj', 'dj',
             'ak', 'dk', 'al', 'dl', 'am', 'dm', 'an', 'dn', 'ao', 'do',
             'ap', 'dp', 'aq', 'ar']

values = range(1, 100)

startVals = {k:'' for k in testOrder}

def recurse(numAssigned, vals):
    print '  '*numAssigned+str(vals)
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
    board = board.replace(' \\ ',' # ') # Remove blank cases

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
        print 'uncertain on {}'.format(numTests)
        return False
