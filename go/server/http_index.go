package server

import (
	"github.com/qmsk/pngtile/go"
	"net/http"
	"path/filepath"
)

const (
	IndexWidth  = 640
	IndexHeight = 320
)

type IndexItem struct {
	Name string
	URL  string

	imageInfo pngtile.ImageInfo
}

func (item IndexItem) Title() string {
	return filepath.Base(item.Name)
}

func (item IndexItem) ImageURL() string {
	var tileParams = TileParams{
		Width:  IndexWidth,
		Height: IndexHeight,
		X:      item.imageInfo.ImageWidth / 2,
		Y:      item.imageInfo.ImageHeight / 2,
	}

	if tileURL, err := TileURL(item.Name, item.imageInfo.CacheFormat, tileParams); err != nil {
		return ""
	} else {
		return tileURL
	}
}

type IndexResponse struct {
	Name  string
	Title string
	Items []IndexItem
}

func (server *Server) HandleIndex(r *http.Request, name string) (httpResponse, error) {
	if names, err := server.Images(name); err != nil {
		return httpResponse{}, err
	} else {
		var indexResponse = IndexResponse{
			Name:  name,
			Title: filepath.Base(name),
		}

		for _, name := range names {
			var indexItem = IndexItem{
				Name: name,
				URL:  server.URL(name),
			}

			if imageInfo, err := server.ImageInfo(name); err == nil {
				indexItem.imageInfo = imageInfo
			}

			indexResponse.Items = append(indexResponse.Items, indexItem)
		}

		return renderResponse(r, server.templates.indexTemplate, indexResponse)
	}
}
