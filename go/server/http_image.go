package server

import (
	"github.com/qmsk/pngtile/go"
	"net/http"
)

type Image struct {
	Name   string
	Config ImageConfig
}

const TileURL = "{tiles_url}?t={tiles_mtime}&x={x}&y={y}&zoom={z}"
const TileSize uint = 256
const TileZoom int = 4

const ImageURL = "{tiles_url}?w={w}&h={h}&x={x}&y={y}&zoom={z}"

type ImageConfig struct {
	TilesURL          string `json:"tiles_url"`
	TilesModifiedTime int    `json:"tiles_mtime"`
	TileURL           string `json:"tile_url"`
	TileSize          uint   `json:"tile_size"`
	TileZoom          int    `json:"tile_zoom"`
	ImageURL          string `json:"image_url"`
	ImageWidth        uint   `json:"image_width"`
	ImageHeight       uint   `json:"image_height"`
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
			Name: name,
			Config: ImageConfig{
				TilesURL:          server.URL(name),
				TilesModifiedTime: 0, // TODO: caching
				TileURL:           TileURL,
				TileSize:          TileSize,
				TileZoom:          TileZoom,
				ImageURL:          ImageURL,
				ImageWidth:        pngtileInfo.ImageWidth,
				ImageHeight:       pngtileInfo.ImageHeight,
			},
		}

		return &image, nil
	}
}

func (server *Server) HandleImage(r *http.Request, name, path string) (httpResponse, error) {
	if image, err := server.Image(name); err != nil {
		return httpResponse{}, err
	} else {
		return renderResponse(r, imageTemplate, image)
	}
}
