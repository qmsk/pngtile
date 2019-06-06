package server

import (
	"github.com/qmsk/pngtile/go"
	"net/http"
	"path/filepath"
	"strings"
)

const (
	IndexWidth  = 640
	IndexHeight = 320
)

type IndexItem struct {
	Name string
}

func (item IndexItem) Title() string {
	if item.Name == "" {
		return "/"
	} else {
		return filepath.Base(item.Name)
	}
}

func (item IndexItem) URL() string {
	if item.Name == "/" {
		return "/"
	} else {
		return "/" + item.Name
	}
}

type IndexImage struct {
	IndexItem

	imageInfo pngtile.ImageInfo
}

func (item IndexImage) ImageURL() string {
	var tileParams = TileParams{
		Width:  IndexWidth,
		Height: IndexHeight,
		X:      item.imageInfo.ImageWidth / 2,
		Y:      item.imageInfo.ImageHeight / 2,
	}

	if tileURL, err := TileURL(item.Name, item.imageInfo.ImageFormat, tileParams); err != nil {
		return ""
	} else {
		return tileURL
	}
}

type IndexResponse struct {
	Name  string
	Title string

	Breadcrumb []IndexItem
	Navigation []IndexItem
	Images     []IndexImage
}

func (index *IndexResponse) buildBreadcrumb(path string) []IndexItem {
	var breadcrumb []IndexItem
	var name = ""

	if path != "" {
		path = "/" + path
	}

	for _, part := range strings.Split(path, "/") {
		if name == "" {
			name = part
		} else {
			name = name + "/" + part
		}

		breadcrumb = append(breadcrumb, IndexItem{
			Name: name + "/",
		})
	}

	return breadcrumb
}

func (server *Server) HandleIndex(r *http.Request, name string) (httpResponse, error) {
	if dirs, imageNames, err := server.List(name); err != nil {
		return httpResponse{}, err
	} else {
		var indexResponse = IndexResponse{
			Name:  name,
			Title: filepath.Base(name),
		}

		indexResponse.Breadcrumb = indexResponse.buildBreadcrumb(name)

		for _, dirName := range dirs {
			indexResponse.Navigation = append(indexResponse.Navigation, IndexItem{
				Name: dirName + "/",
			})
		}

		for _, imageName := range imageNames {
			var indexImage = IndexImage{
				IndexItem: IndexItem{
					Name: imageName,
				},
			}

			if imageInfo, err := server.ImageInfo(imageName); err == nil {
				indexImage.imageInfo = imageInfo
			}

			indexResponse.Images = append(indexResponse.Images, indexImage)
		}

		return renderResponse(r, server.templates.indexTemplate, indexResponse)
	}
}
