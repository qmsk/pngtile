package server

import (
	"github.com/qmsk/pngtile/go"
	"io/ioutil"
	"path/filepath"
	"strings"
)

func (server *Server) List(name string) (dirs []string, images []string, err error) {
	if path, err := server.Path(name, "/"); err != nil {
		return dirs, images, err
	} else if files, err := ioutil.ReadDir(path); err != nil {
		return dirs, images, err
	} else {
		for _, file := range files {
			var fileBase = file.Name()
			var filePath = filepath.Join(path, fileBase)
			var fileName = filepath.Join(name, strings.Split(fileBase, ".")[0])

			if strings.HasPrefix(file.Name(), ".") {
				continue
			}

			if file.Mode().IsDir() {
				dirs = append(dirs, fileName)
			} else if !file.Mode().IsRegular() {

			} else if imageFormat, ok, err := pngtile.SniffImage(filePath); err != nil {
				return dirs, images, err
			} else if !ok {
				continue
			} else if imageFormat == pngtile.FORMAT_CACHE {
				images = append(images, fileName)
			}
		}

		return dirs, images, nil
	}
}

func (server *Server) Images(name string) ([]string, error) {
	if path, err := server.Path(name, "/"); err != nil {
		return nil, err
	} else if scanImages, err := pngtile.Scan(path, pngtile.ScanOptions{OnlyCached: true}); err != nil {
		return nil, err
	} else {
		var names = make([]string, 0)

		for _, scanImage := range scanImages {
			var name = scanImage.CachePath

			// relative to server path
			name = strings.TrimPrefix(name, server.path)

			// bare name without the .cache suffix
			name = strings.Split(name, ".")[0]

			names = append(names, name)
		}

		return names, nil
	}
}

type Image struct {
	path         string
	pngtileImage *pngtile.Image
	pngtileInfo  pngtile.ImageInfo
}

func (server *Server) openImage(name string) (*Image, error) {
	if path, err := server.Path(name, ".cache"); err != nil {
		return nil, err
	} else if pngtileImage, err := pngtile.OpenImage(path); err != nil {
		return nil, err
	} else if err := pngtileImage.Open(); err != nil {
		return nil, err
	} else if pngtileInfo, err := pngtileImage.Info(); err != nil {
		return nil, err
	} else {
		var image = Image{
			path:         path,
			pngtileImage: pngtileImage,
			pngtileInfo:  pngtileInfo,
		}

		return &image, nil
	}
}

func (server *Server) image(name string) (*Image, error) {
	// XXX: unsafe map access
	if image := server.images[name]; image != nil {
		return image, nil
	} else if image, err := server.openImage(name); err != nil {
		return nil, err
	} else {
		server.images[name] = image

		return image, nil
	}
}

func (server *Server) ImageInfo(name string) (pngtile.ImageInfo, error) {
	if image, err := server.image(name); err != nil {
		return pngtile.ImageInfo{}, err
	} else {
		return image.pngtileInfo, nil
	}
}

func (server *Server) ImageTile(name string, params pngtile.TileParams) ([]byte, error) {
	if image, err := server.image(name); err != nil {
		return nil, err
	} else if tileData, err := image.pngtileImage.Tile(params); err != nil {
		return nil, err
	} else {
		return tileData, nil
	}
}
