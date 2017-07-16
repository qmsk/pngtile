package server

import (
	"net/http"
)

type ImageResponse struct {
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
	ImageFormat  string `json:"image_format"`
	ImageWidth   uint   `json:"image_width"`
	ImageHeight  uint   `json:"image_height"`
}

func (server *Server) HandleImage(r *http.Request, name string) (httpResponse, error) {
	if imageInfo, err := server.ImageInfo(name); err != nil {
		return httpResponse{}, err
	} else {
		return renderResponse(r, imageTemplate, ImageResponse{
			Name: name,
			Config: ImageConfig{
				URL:          server.URL(name + "." + imageInfo.CacheFormat.String()),
				ModifiedTime: 0, // TODO: caching
				TileURL:      TileURL,
				TileSize:     TileSize,
				TileZoom:     TileZoomMax,
				ViewURL:      ViewURL,
				ImageFormat:  imageInfo.CacheFormat.String(),
				ImageWidth:   imageInfo.ImageWidth,
				ImageHeight:  imageInfo.ImageHeight,
			},
		})
	}
}
