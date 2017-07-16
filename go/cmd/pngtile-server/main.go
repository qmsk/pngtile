package main

import (
	flags "github.com/jessevdk/go-flags"
	web "github.com/qmsk/go-web"
	server "github.com/qmsk/pngtile/go/server"

	"log"
)

var options struct {
	Web web.Options `group:"Web"`

	Path string `long:"pngtile-path"`
}

func main() {
	if args, err := flags.Parse(&options); err != nil {
		log.Fatalf("flags.Parse: %v", err)
	} else if len(args) > 0 {
		log.Fatalf("Usage: [options]")
	} else {

	}

	var config = server.Config{
		Path: options.Path,
	}

	if server, err := config.MakeServer(); err != nil {
		log.Fatalf("server:Config.MakeServer: %v", err)
	} else {
		options.Web.Server(
			options.Web.RouteStatic("/static/"),
			options.Web.Route("/", server),
		)
	}
}
