package server

import (
	"fmt"
	"github.com/gorilla/schema"
	"github.com/qmsk/pngtile/go"
	"log"
	"net/http"
	"net/url"
)

const TileSize uint = 256
const TileZoomMin int = 0
const TileZoomMax int = 4
const TileLimit uint = 1920 * 1200

const TileURL = "{url}?t={mtime}&tile-x={x}&tile-y={y}&zoom={z}" // tile image: xy are in units of scaled tiles
const ViewURL = "{url}?w={w}&h={h}&x={x}&y={y}&zoom={z}"         // fullscreen/linked view image: xy is the scaled center point of the wh viewport

type TileParams struct {
	Time   int  `schema:"t"`
	Width  uint `schema:"w"`
	Height uint `schema:"h"`
	TileX  uint `schema:"tile-x"`
	TileY  uint `schema:"tile-y"`
	X      uint `schema:"x"`
	Y      uint `schema:"y"`
	Zoom   int  `schema:"zoom"`
}

// Scale coordinate by zoom level
func (params TileParams) zoomScale(xy uint) uint {
	if params.Zoom > 0 {
		return xy << uint(params.Zoom)
	} else if params.Zoom < 0 {
		return xy >> uint(-params.Zoom)
	} else {
		return xy
	}
}

// Scale view center coordinate to view width/height
func (params TileParams) zoomScaleCentered(xy uint, wh uint) uint {
	if xy > wh/2 {
		return params.zoomScale(xy - wh/2)
	} else {
		return 0
	}
}

func (params TileParams) tileParams(imageInfo pngtile.ImageInfo) (pngtile.TileParams, error) {
	var tileParams = pngtile.TileParams{
		Zoom: params.Zoom,
	}

	if params.TileX != 0 || params.TileY != 0 {
		// normal tile
		tileParams.Width = TileSize
		tileParams.Height = TileSize
		tileParams.X = params.zoomScale(params.TileX * TileSize)
		tileParams.Y = params.zoomScale(params.TileY * TileSize)
	} else if params.Width != 0 && params.Height != 0 {
		// centered view
		tileParams.Width = params.Width
		tileParams.Height = params.Height
		tileParams.X = params.zoomScaleCentered(params.X, params.Width)
		tileParams.Y = params.zoomScaleCentered(params.Y, params.Height)
	} else {
		return tileParams, fmt.Errorf("Invalid parameters: use either ?tx=&ty= or ?w=&h=")
	}

	log.Printf("%#v image=%#v -> %#v", params, imageInfo, tileParams)

	if params.Zoom > TileZoomMax || params.Zoom < TileZoomMin {
		return tileParams, fmt.Errorf("Invalid zoom level: %d (limit %d-%d)", params.Zoom, TileZoomMin, TileZoomMax)
	}

	var tileSize = tileParams.Width * tileParams.Height
	if tileSize > TileLimit {
		return tileParams, fmt.Errorf("Invalid width x height: %d (limit %d)", tileSize, TileLimit)
	}

	return tileParams, nil
}

func renderResponseTile(image *Image, params TileParams) (httpResponse, error) {
	if tileParams, err := params.tileParams(image.pngtileInfo); err != nil {
		return httpResponse{Status: 400}, err
	} else if tileData, err := image.pngtileImage.Tile(tileParams); err != nil {
		return httpResponse{}, err
	} else {
		return renderResponsePNG(tileData)
	}
}

func (server *Server) HandleImageTile(r *http.Request, name string, query url.Values) (httpResponse, error) {
	var params TileParams

	if err := schema.NewDecoder().Decode(&params, query); err != nil {
		return httpResponse{Status: 400}, err
	} else if image, err := server.Image(name); err != nil {
		return httpResponse{}, err
	} else {
		return renderResponseTile(image, params)
	}
}
