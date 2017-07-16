package server

import (
	"fmt"
	"github.com/qmsk/pngtile/go"
	"path/filepath"
)

func makeServer(config Config) (*Server, error) {
	var server = Server{
		config: config,

		path:   filepath.Clean(config.Path) + "/",
		images: make(map[string]*pngtile.Image),
	}

	return &server, nil
}

type Server struct {
	config Config
	path   string
	images map[string]*pngtile.Image
}

func (server *Server) URL(name string) string {
	return "/" + name
}

func (server *Server) Path(name string) (string, error) {
	if name == "" {
		return server.path, nil
	}

	var path = filepath.Clean(filepath.Join(server.path, name))

	if !filepath.HasPrefix(path, server.path) {
		return path, fmt.Errorf("Invalid path: %v", name)
	}

	return path, nil
}
