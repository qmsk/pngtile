package server

import (
	"fmt"
	"path/filepath"
	"strings"
)

func makeServer(config Config) (*Server, error) {
	var server = Server{
		config: config,

		path:   filepath.Clean(config.Path) + "/",
		images: make(map[string]*Image),
	}

	if templates, err := server.loadTemplates(); err != nil {
		return nil, err
	} else {
		server.templates = templates
	}

	return &server, nil
}

type Server struct {
	config Config
	path   string

	templates templates
	images    map[string]*Image
}

func (server *Server) URL(name string) string {
	return "/" + name
}

func (server *Server) Path(url, suffix string) (string, error) {
	if url == "" {
		return server.path, nil
	}

	var path = filepath.Clean(filepath.Join(server.path, url))

	if !filepath.HasPrefix(path, server.path) {
		return path, fmt.Errorf("Invalid path: %v", url)
	}

	if suffix != "" {
		path = path + suffix
	}

	return path, nil
}

func (server *Server) Lookup(url string, defaultExt string) (path, name, ext string, err error) {
	var dir, base string

	if url == "" {
		dir = "/"
		base = ""
		name = ""
	} else if strings.HasSuffix(url, "/") {
		dir = url
		base = ""
		name = strings.TrimSuffix(url, "/")
	} else {
		dir = filepath.Dir(url)
		base = filepath.Base(url)

		var parts = strings.SplitN(base, ".", 2)

		if dir == "." {
			name = parts[0]
		} else {
			name = dir + "/" + parts[0]
		}

		if len(parts) > 1 {
			ext = parts[1]
		}
	}

	path, err = server.Path(url, "")

	if base != "" && ext == "" {
		path = path + "." + defaultExt
	}

	return path, name, ext, err
}
