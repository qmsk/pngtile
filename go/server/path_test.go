package server

import (
	"github.com/stretchr/testify/assert"
	"testing"
)

func testServer(config Config) *Server {
	if server, err := makeServer(config); err != nil {
		panic(err)
	} else {
		return server
	}
}

var testServerPath = []struct {
	url string
	ext string

	path string
	err  error
}{
	{"", "", "test/", nil},
	{"foo", "", "test/foo", nil},
	{"foo", "/", "test/foo/", nil},
	{"foo", ".cache", "test/foo.cache", nil},
}

func TestServerPath(t *testing.T) {
	var server = testServer(Config{Path: "./test"})

	for _, test := range testServerPath {
		path, err := server.Path(test.url, test.ext)

		if test.err == err {
			assert.Equal(t, test.path, path, "path %s ext=%s", test.url, test.ext)
		} else if test.err == nil {
			assert.Error(t, err, "path %s ext=%s", test.url, test.ext)
		} else {
			assert.NoError(t, err, "path %s ext=%s", test.url, test.ext)
		}
	}
}

var testServerLookup = []struct {
	url        string
	defaultExt string

	path string
	name string
	ext  string
	err  error
}{
	{"", "", "test/", "", "", nil},
	{"foo/", "cache", "test/foo", "foo", "", nil},
	{"foo", "cache", "test/foo.cache", "foo", "", nil},
	{"foo.png", "cache", "test/foo.png", "foo", "png", nil},
}

func TestServerLookup(t *testing.T) {
	var server = testServer(Config{Path: "./test"})

	for _, test := range testServerLookup {
		path, name, ext, err := server.Lookup(test.url, test.defaultExt)

		if test.err == err {
			assert.Equal(t, test.path, path, "path %s ext=%s", test.url, test.ext)
			assert.Equal(t, test.name, name, "name %s ext=%s", test.url, test.ext)
			assert.Equal(t, test.ext, ext, "ext %s ext=%s", test.url, test.ext)
		} else if test.err == nil {
			assert.Error(t, err, "path %s ext=%s", test.url, test.ext)
		} else {
			assert.NoError(t, err, "path %s ext=%s", test.url, test.ext)
		}
	}
}
