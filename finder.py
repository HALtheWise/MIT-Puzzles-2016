f = open("sowpods.txt")
sowpods = set([s.strip() for s in f.readlines()])

def findWords(s):
    results = []
    for w in sowpods:
        if w in s:
            results.append((w,s.index(w)))

    return sorted(results,key=lambda x:-len(x[0]))
