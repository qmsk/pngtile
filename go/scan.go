package pngtile

import (
	"os"
	"path/filepath"
	"strings"
)

// Recursively scan given directory for image files
func Scan(root string) ([]string, error) {
	var paths []string

	return paths, filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if !strings.HasPrefix(path, ".") {

		} else if info.IsDir() {
			return filepath.SkipDir
		} else {
			return nil
		}

		if !info.Mode().IsRegular() {
			return nil
		}

		if ok, err := SniffImage(path); err != nil {
			return err
		} else if ok {
			paths = append(paths, path)
		}

		return nil
	})
}
