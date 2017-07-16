package server

import (
	"bytes"
	"encoding/json"
	"fmt"
	"html/template"
	"log"
	"net/http"
	"os"
	"strings"
)

type httpResponse struct {
	Status      int
	ContentType string
	Content     []byte
}

func renderResponsePNG(data []byte) (httpResponse, error) {
	return httpResponse{200, "image/png", data}, nil
}

func renderResponseJSON(data interface{}) (httpResponse, error) {
	var buffer bytes.Buffer

	if err := json.NewEncoder(&buffer).Encode(data); err != nil {
		return httpResponse{}, err
	} else {
		return httpResponse{200, "application/json", buffer.Bytes()}, nil
	}
}

func renderResponseHTML(data interface{}, template *template.Template) (httpResponse, error) {
	var buffer bytes.Buffer

	if err := template.Execute(&buffer, data); err != nil {
		return httpResponse{}, err
	} else {
		return httpResponse{200, "text/html", buffer.Bytes()}, nil
	}
}

func renderResponse(r *http.Request, template *template.Template, data interface{}) (httpResponse, error) {
	for _, accept := range strings.Split(r.Header.Get("Accept"), ",") {
		switch accept {
		case "text/html":
			return renderResponseHTML(data, template)
		case "application/json":
			return renderResponseJSON(data)
		default:
			continue
		}
	}

	return renderResponseHTML(data, template)
}

func (server *Server) Handle(r *http.Request) (httpResponse, error) {
	path, name, ext, err := server.Lookup(r.URL.Path, "cache")

	log.Printf("%s %s = %s (name=%s ext=%s) (error: %v)", r.Method, r.URL.Path, path, name, ext, err)

	if r.Method != "GET" {
		return httpResponse{Status: http.StatusNotImplemented}, err
	} else if err != nil {
		return httpResponse{Status: 403}, err
	} else if stat, err := os.Stat(path); err != nil {
		if os.IsNotExist(err) {
			return httpResponse{Status: 404}, err
		} else {
			return httpResponse{}, err
		}
	} else if stat.Mode().IsDir() {
		return server.HandleIndex(r, name)
	} else if ext == "" {
		return server.HandleImage(r, name)
	} else if ext == "png" {
		return server.HandleImageTile(r, name, r.URL.Query())
	} else {
		return httpResponse{Status: 404}, err
	}
}

func (server *Server) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	if response, err := server.Handle(r); err == nil {
		w.Header().Set("Content-Type", response.ContentType)
		w.WriteHeader(response.Status)

		w.Write(response.Content)

	} else {
		var status = response.Status

		if status == 0 {
			status = 500
		}

		w.Header().Set("Content-Type", "text/plain")
		w.WriteHeader(status)

		fmt.Fprintf(w, "%v", err)
	}
}
