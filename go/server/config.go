package server

import (
	"path/filepath"
)

type Config struct {
	TemplatePath string
	Path         string
}

func (config Config) MakeServer() (*Server, error) {
	return makeServer(config)
}

func (config Config) templatePath(name string) string {
	return filepath.Join(config.TemplatePath, name)
}
