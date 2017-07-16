package server

import (
	"github.com/qmsk/pngtile/go"
	"net/http"
)

type Image struct {
	Name string
	Info pngtile.ImageInfo
}

func (server *Server) Image(name, path string) (*Image, error) {
	if pngtileImage, err := pngtile.OpenImage(path, pngtile.OPEN_READ); err != nil {
		return nil, err
	} else if err := pngtileImage.Open(); err != nil {
		return nil, err
	} else if info, err := pngtileImage.Info(); err != nil {
		return nil, err
	} else {
		var image = Image{
			Name: name,
			Info: info,
		}

		return &image, nil
	}
}

func (server *Server) HandleImage(r *http.Request, name, path string) (httpResponse, error) {
	if image, err := server.Image(name, path); err != nil {
		return httpResponse{}, err
	} else {
		return renderResponse(r, imageTemplate, image)
	}
}
