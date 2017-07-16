package server

import (
	"net/http"
)

type IndexResponse struct {
	Name  string
	Names []string
}

func (server *Server) HandleIndex(r *http.Request, name string) (httpResponse, error) {
	if names, err := server.Images(name); err != nil {
		return httpResponse{}, err
	} else {
		return renderResponse(r, indexTemplate, IndexResponse{
			Name:  name,
			Names: names,
		})
	}
}
