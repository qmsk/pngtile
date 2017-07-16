package pngtile

/*
#include "pngtile.h"
*/
import "C"

type TileParams struct {
	Width, Height uint
	X, Y          uint
	Zoom          int
}

func (params TileParams) c_struct() C.struct_pt_tile_params {
	var tile_params C.struct_pt_tile_params

	tile_params.width = C.uint(params.Width)
	tile_params.height = C.uint(params.Height)
	tile_params.x = C.uint(params.X)
	tile_params.y = C.uint(params.Y)
	tile_params.zoom = C.int(params.Zoom)

	return tile_params
}
