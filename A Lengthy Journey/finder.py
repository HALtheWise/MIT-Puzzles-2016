import re
import itertools

f = open("code.txt")
words = set([s.strip().lower() for s in f.readlines()])
f.close()

english = dict([(n,[]) for n in range(1,100)])

f = open("words.txt")
for l in f.readlines():
    s = l.strip()
    if len(s)<=1:
        continue
    english[len(s)].append(s.lower())
f.close()

def tokenizePattern(string):
    return re.match("(\d)+([^\d])*")


def couldMatchPattern(string, pattern):
    match=re.match("(\d+)([^\d]*)", pattern)
    
    for w in english[int(match.groups()[0])]:
        if string.lower().startswith(w):
            if len(w) == len(string):
                return True

            # recurse
            newPattern = pattern[len(match.group()):]
            newString = string[len(w):]
            if couldMatchPattern(newString, newPattern):
                return True

    return False

def couldBeEnglish(string):
    for l in english:
        for w in english[l]:
            if string.lower().startswith(w):
                return True

    return False


def generatePermutations(basestr='', depth=2):
##    if not couldBeEnglish(basestr) and basestr:
##        return
    if depth <= 0:
        yield basestr
        return
    for w in words:
        it = generatePermutations(basestr+w, depth-1)
        for i in it:
            yield i
