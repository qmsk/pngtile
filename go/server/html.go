package server

import (
	"html/template"
)

type templates struct {
	baseTemplate  *template.Template
	indexTemplate *template.Template
	imageTemplate *template.Template
}

func (server *Server) loadTemplates() (templates, error) {
	var templates templates

	if indexTemplate, err := template.ParseFiles(server.config.templatePath("pngtile.html"), server.config.templatePath("index.html")); err != nil {
		return templates, err
	} else {
		templates.indexTemplate = indexTemplate
	}

	if imageTemplate, err := template.ParseFiles(server.config.templatePath("pngtile.html"), server.config.templatePath("image.html")); err != nil {
		return templates, err
	} else {
		templates.imageTemplate = imageTemplate
	}

	return templates, nil
}
