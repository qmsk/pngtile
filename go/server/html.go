package server

import (
	"html/template"
)

const baseHTML = `
<html>
  <head>
    <title>{{block "title" .}}pngtile{{end}}</title>
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
{{define "body"}}
  TODO
{{end}}
`

func init() {
	template.Must(indexTemplate.Parse(baseHTML))
	template.Must(indexTemplate.Parse(indexHTML))

	template.Must(imageTemplate.Parse(baseHTML))
	template.Must(imageTemplate.Parse(imageHTML))
}
