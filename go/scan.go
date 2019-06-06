package pngtile

import (
	"os"
	"path/filepath"
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
	ImagePath string
	CachePath string
	Format    ImageFormat
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
