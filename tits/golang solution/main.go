// tits project main.go
package main

import (
	"bufio"
	"fmt"
	"hash/crc64"
	"os"
)

var explored map[uint64]struct{}
var frontier map[string]uint64

var table *crc64.Table

func init() {
	table = crc64.MakeTable(crc64.ISO)
}

func processValue(v string, resultsFile *bufio.Writer) {

	if couldBeRight(v) {
		fmt.Fprintln(resultsFile, v)
		fmt.Println("May have found one!")
	}

	explored[frontier[v]] = struct{}{}
	delete(frontier, v)
	for _, t := range ops {
		newstr := Apply(t, v)
		h := crc64.Checksum([]byte(newstr), table)
		//		fmt.Println(h, newstr)
		if _, ok := explored[h]; ok {
			// This has been found before
			continue
		} else {
			frontier[newstr] = h
		}
	}
}

const numResults = 20055000

func generateAll() {
	frontier = make(map[string]uint64)
	explored = make(map[uint64]struct{}, numResults+1e5)

	file, _ := os.Create("interest.txt")
	defer file.Close()
	writer := bufio.NewWriter(file)
	defer writer.Flush()

	// Initialize frontier
	frontier[scrambledMessage] = crc64.Checksum([]byte(scrambledMessage), table)

	for len(frontier) > 0 {
		for v := range frontier {
			processValue(v, writer)
			if len(explored)%10000 == 0 {
				fmt.Printf("%.1f%% done... frontier: %d, explored: %d\n",
					float64(len(explored))/numResults*100, len(frontier), len(explored))
			}
		}
	}
}

func main() {
	fmt.Println("Hello World!")
	fmt.Println(couldBeRight(scrambledMessage))
	fmt.Println(pA)
	generateAll()
}
