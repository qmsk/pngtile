package server

import (
	"fmt"
	"github.com/gorilla/schema"
	"github.com/qmsk/pngtile/go"
	"net/http"
	"net/url"
)

type TileParams struct {
	Time   int  `schema:"t"`
	Width  uint `schema:"width"`
	Height uint `schema:"height"`
	X      uint `schema:"x"`
	Y      uint `schema:"y"`
	Zoom   int  `schema:"zoom"`
}

func (params TileParams) scale(xy uint) uint {
	if params.Zoom > 0 {
		return xy << uint(params.Zoom)
	} else if params.Zoom < 0 {
		return xy >> uint(-params.Zoom)
	} else {
		return xy
	}
}

func (params TileParams) scaleCenter(xy uint, wh uint) uint {
	return params.scale(xy - wh/2)
}

func (params TileParams) renderParams(imageInfo pngtile.ImageInfo) (pngtile.TileParams, error) {
	var tileParams = pngtile.TileParams{
		Zoom: params.Zoom,
	}

	if params.Width > 0 && params.Height > 0 {
		// centered region
		tileParams.X = params.scaleCenter(params.X, imageInfo.ImageWidth)
		tileParams.Y = params.scaleCenter(params.Y, imageInfo.ImageHeight)
	} else {
		// normal tile
		tileParams.Width = TileSize
		tileParams.Height = TileSize
		tileParams.X = params.scale(params.X * TileSize)
		tileParams.Y = params.scale(params.Y * TileSize)
	}

	if params.Zoom > TileZoomMax || params.Zoom < TileZoomMin {
		return tileParams, fmt.Errorf("Invalid zoom level: %d (limit %d-%d)", params.Zoom, TileZoomMin, TileZoomMax)
	}

	var tileSize = tileParams.Width * tileParams.Height
	if tileSize > TileLimit {
		return tileParams, fmt.Errorf("Invalid width x height: %d (limit %d)", tileSize, TileLimit)
	}

	return tileParams, nil
}

const TileURL = "{tiles_url}?t={tiles_mtime}&x={x}&y={y}&zoom={z}"
const TileSize uint = 256
const TileZoomMin int = 0
const TileZoomMax int = 4
const TileLimit uint = 1920 * 1200

const ImageURL = "{tiles_url}?w={w}&h={h}&x={x}&y={y}&zoom={z}"

func renderResponseTile(image *Image, params TileParams) (httpResponse, error) {
	if tileParams, err := params.renderParams(image.pngtileInfo); err != nil {
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
