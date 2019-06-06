package pngtile

import (
	"fmt"
	"log"
	"os"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
)

type ScanOptions struct {
	IncludeCached bool // include FORMAT_CACHE
	OnlyCached    bool // only FORMAT_CACHE
}

func (scanOptions ScanOptions) filter(scanImage ScanImage) bool {
	if scanOptions.OnlyCached {
		return scanImage.IsCache()
	} else if !scanOptions.IncludeCached {
		return !scanImage.IsCache()
	}

	return true
}

type ScanImage struct {
	ImagePath  string
	ImagePaths [][]string
	CachePath  string
	Format     ImageFormat
}

func (scanImage ScanImage) IsCache() bool {
	return scanImage.Format == FORMAT_CACHE
}

func ScanFile(path string) (scanImage ScanImage, ok bool, err error) {
	if imageFormat, ok, err := SniffImage(path); err != nil {
		return scanImage, false, err
	} else if !ok {
		return scanImage, false, nil
	} else {
		scanImage.Format = imageFormat
	}

	if scanImage.Format == FORMAT_CACHE {
		scanImage.CachePath = path
	} else if cachePath, err := CachePath(path); err != nil {
		// ignore
		return scanImage, false, nil
	} else {
		scanImage.ImagePath = path
		scanImage.CachePath = cachePath
	}

	return scanImage, true, nil
}

// Recursively scan given directory for image files
//
// This will will return either cached files, or image files that need to be cached.
func Scan(root string, options ScanOptions) ([]ScanImage, error) {
	var images []ScanImage

	return images, filepath.Walk(root, func(path string, info os.FileInfo, err error) error {
		if !strings.HasPrefix(path, ".") {

		} else if info.IsDir() {
			return filepath.SkipDir
		} else {
			return nil
		}

		if !info.Mode().IsRegular() {
			return nil
		}

		if scanImage, ok, err := ScanFile(path); err != nil {
			return err
		} else if !ok {
			return nil
		} else if options.filter(scanImage) {
			images = append(images, scanImage)
		}

		return nil
	})
}

type scanPart struct {
	path, ext string
	row, col  int
}

func ScanParts(paths []string, pattern *regexp.Regexp) (ScanImage, error) {
	var scanImage ScanImage
	var path, ext string
	var partsMap = make(map[scanPart]string)
	var rows = 0
	var cols = 0

	for _, p := range paths {
		var scanPart scanPart

		if matches := pattern.FindStringSubmatch(p); matches == nil {
			log.Printf("scan path %s: skip", p)
			// skip
			continue
		} else {
			scanPart.path = matches[1]
			scanPart.ext = matches[4]

			if row, err := strconv.ParseInt(matches[2], 0, 0); err != nil {
				return scanImage, fmt.Errorf("Invalid row for path %s: %s", p, err)
			} else {
				scanPart.row = int(row)
			}

			if col, err := strconv.ParseInt(matches[3], 0, 0); err != nil {
				return scanImage, fmt.Errorf("Invalid col for path %s: %s", p, err)
			} else {
				scanPart.col = int(col)
			}
		}

		if imageFormat, ok, err := SniffImage(p); err != nil {
			return scanImage, fmt.Errorf("Failed image %s: %s", p, err)
		} else if !ok {
			return scanImage, fmt.Errorf("Invalid image %s", p)
		} else {
			scanImage.Format = imageFormat
		}

		log.Printf("scan path %s: %#v", p, scanPart)

		partsMap[scanPart] = p

		path = scanPart.path
		ext = scanPart.ext

		if scanPart.row > rows {
			rows = scanPart.row
		}
		if scanPart.col > cols {
			cols = scanPart.col
		}
	}

	scanImage.ImagePath = path + "." + ext
	scanImage.ImagePaths = make([][]string, rows)

	for row := 0; row < rows; row++ {
		scanImage.ImagePaths[row] = make([]string, cols)

		for col := 0; col < cols; col++ {
			scanImage.ImagePaths[row][col] = partsMap[scanPart{path, ext, row + 1, col + 1}]
		}
	}

	if cachePath, err := CachePath(scanImage.ImagePath); err != nil {
		return scanImage, err
	} else {
		scanImage.CachePath = cachePath
	}

	return scanImage, nil
}
