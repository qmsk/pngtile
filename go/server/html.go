package server

import (
	"html/template"
)

const baseHTML = `
<html>
  <head>
  {{block "head" .}}
    <title>{{block "title" .}}pngtile{{end}}</title>

    {{block "link" .}}
    <link rel="stylesheet" href="/static/app/pngtile.css">
    {{end}}
  {{end}}
  </head>
  <body>
  {{block "body" .}}
    <h1>{{template "title"}}</h1>

  {{end}}
  </body>
</html>
`

var indexTemplate = template.New("index")

const indexHTML = `
{{define "title"}}pngtile :: {{.Name}}{{end}}
{{define "body"}}
<h1>{{template "title"}}</h1>
<ul>
{{range $name := .Names}}
  <li><a href="{{$name}}">{{$name}}</a></li>
{{end}}
</ul>
{{end}}
`

var imageTemplate = template.New("image")

const imageHTML = `
{{define "title"}}pngtile :: {{.Name}}{{end}}
{{define "link"}}
  <link rel="stylesheet" href="/static/app/pngtile.css">
  <link rel="stylesheet" href="/static/app/pngtile/map.css">
  <link rel="stylesheet" href="/static/node_modules/leaflet/dist/leaflet.css">
{{end}}
{{define "body"}}
  <script src="/static/node_modules/jquery/dist/jquery.min.js"></script>
  <script src="/static/node_modules/leaflet/dist/leaflet.js"></script>
  <script src="/static/app/pngtile/map.js"></script>

  <div id="map">

  </div>
  <script>
      $(function () {
          map_init({{.Config}});
      });
  </script>

{{end}}
`

func init() {
	template.Must(indexTemplate.Parse(baseHTML))
	template.Must(indexTemplate.Parse(indexHTML))

	template.Must(imageTemplate.Parse(baseHTML))
	template.Must(imageTemplate.Parse(imageHTML))
}
