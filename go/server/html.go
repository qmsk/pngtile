package server

import (
	"html/template"
)

const baseHTML = `
<html>
  <head>
    <title>{{block "title" .}}pngtile{{end}}</title>
    {{block "head" .}}
    <link rel="stylesheet" href="/static/app/pngtile.css">
    {{end}}
  </head>
  <body>
    <h1>{{template "title"}}</h1>
    {{block "body" .}}

    {{end}}
  </body>
</html>
`

var indexTemplate = template.New("index")

const indexHTML = `
{{define "title"}}pngtile :: {{.Name}}{{end}}
{{define "body"}}
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
{{define "head"}}
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
