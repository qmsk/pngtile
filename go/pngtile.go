package pngtile

// Does this file look like a PNG file that we can use?
func SniffImage(path string) (format ImageFormat, valid bool, err error) {
	return imageSniff(path)
}

// Convert PNG path to .cache path for use with Image
func CachePath(path string) (string, error) {
	return cachePath(path)
}

// Danger: must call image.Close() to not leak.
func OpenImage(cachePath string) (*Image, error) {
	return imageOpen(cachePath)
}

// Open image, and pass to func, or return error.
//
// Returns error from func, releasing the image.
func WithImage(cachePath string, f func(*Image) error) error {
	if image, err := imageOpen(cachePath); err != nil {
		return err
	} else {
		defer image.destroy()

		return f(image)
	}
}
