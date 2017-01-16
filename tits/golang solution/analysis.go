// analysis
package main

import "strings"

func couldBeRight(s string) bool {
	return strings.Index(s, "  ") == -1
}
