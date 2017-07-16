package server

import (
	"github.com/qmsk/pngtile/go"
	"strings"
)

type Image struct {
	path         string
	pngtileImage *pngtile.Image
	pngtileInfo  pngtile.ImageInfo
}

func (server *Server) openImage(name string) (*Image, error) {
	if path, err := server.Path(name); err != nil {
		return nil, err
	} else if pngtileImage, err := pngtile.OpenImage(path, pngtile.OPEN_READ); err != nil {
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

func (server *Server) Images(name string) ([]string, error) {
	if path, err := server.Path(name); err != nil {
		return nil, err
	} else if paths, err := pngtile.Scan(path); err != nil {
		return nil, err
	} else {
		var names []string

		for _, path := range paths {
			var name = strings.TrimPrefix(path, server.path)

			names = append(names, name)
		}

		return names, nil
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
