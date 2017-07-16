package pngtile

// Danger: must call image.Close() to not leak.
func OpenImage(path string, mode OpenMode) (*Image, error) {
	return imageOpen(path, mode)
}

// Open image, and pass to func, or return error.
//
// Returns error from func, releasing the image.
func WithImage(path string, mode OpenMode, f func(*Image) error) error {
	if image, err := imageOpen(path, mode); err != nil {
		return err
	} else {
		defer image.destroy()

		return f(image)
	}
}
