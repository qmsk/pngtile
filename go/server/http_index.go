package server

import (
	"github.com/qmsk/pngtile/go"
	"net/http"
	"strings"
)

type Index struct {
	Name  string
	Names []string
}

func (server *Server) Index(name, path string) (*Index, error) {
	if paths, err := pngtile.Scan(path); err != nil {
		return nil, err
	} else {
		var names []string

		for _, path := range paths {
			var name = strings.TrimPrefix(path, server.path)

			names = append(names, name)
		}

		var index = Index{
			Name:  name,
			Names: names,
		}

		return &index, nil
	}
}

func (server *Server) HandleIndex(r *http.Request, name, path string) (httpResponse, error) {
	if index, err := server.Index(name, path); err != nil {
		return httpResponse{}, err
	} else {
		return renderResponse(r, indexTemplate, index)
	}
}
