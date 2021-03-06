package main

import (
	flags "github.com/jessevdk/go-flags"
	web "github.com/qmsk/go-web"
	pngtile "github.com/qmsk/pngtile/go"
	server "github.com/qmsk/pngtile/go/server"

	"log"
)

var options struct {
	Web web.Options `group:"Web"`

	Debug        bool   `long:"pngtile-debug"`
	Quiet        bool   `long:"pngtile-quiet"`
	Path         string `long:"pngtile-path"`
	TemplatePath string `long:"pngtile-templates" default:"web/templates"`
}

func main() {
	if args, err := flags.Parse(&options); err != nil {
		log.Fatalf("flags.Parse: %v", err)
	} else if len(args) > 0 {
		log.Fatalf("Usage: [options]")
	} else {

	}

	pngtile.LogDebug(options.Debug)
	pngtile.LogWarn(!options.Quiet)

	var config = server.Config{
		Path:         options.Path,
		TemplatePath: options.TemplatePath,
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
