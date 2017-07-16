package server

type Config struct {
	Path string
}

func (config Config) MakeServer() (*Server, error) {
	return makeServer(config)
}
