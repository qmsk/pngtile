package server

import (
	"github.com/qmsk/pngtile/go"
	"net/http"
)

type Image struct {
	path         string
	pngtileImage *pngtile.Image
	pngtileInfo  pngtile.ImageInfo

	Name   string
	Config ImageConfig
}

type ImageConfig struct {
	URL          string `json:"url"`
	ModifiedTime int    `json:"mtime"`
	TileURL      string `json:"tile_url"`
	TileSize     uint   `json:"tile_size"`
	TileZoom     int    `json:"tile_zoom"`
	ViewURL      string `json:"view_url"`
	ImageWidth   uint   `json:"image_width"`
	ImageHeight  uint   `json:"image_height"`
}

func (server *Server) Image(name string) (*Image, error) {
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

			Name: name,
			Config: ImageConfig{
				URL:          server.URL(name),
				ModifiedTime: 0, // TODO: caching
				TileURL:      TileURL,
				TileSize:     TileSize,
				TileZoom:     TileZoomMax,
				ViewURL:      ViewURL,
				ImageWidth:   pngtileInfo.ImageWidth,
				ImageHeight:  pngtileInfo.ImageHeight,
			},
		}

		return &image, nil
	}
}

func (server *Server) HandleImage(r *http.Request, name string) (httpResponse, error) {
	if image, err := server.Image(name); err != nil {
		return httpResponse{}, err
	} else {
		return renderResponse(r, imageTemplate, image)
	}
}
