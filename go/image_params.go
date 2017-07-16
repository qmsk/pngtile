package pngtile

/*
#include "pngtile.h"
*/
import "C"

type ImagePixel [4]uint8

type ImageParams struct {
	BackgroundPixel *ImagePixel
}

func (params ImageParams) image_params() C.struct_pt_image_params {
	var image_params C.struct_pt_image_params

	if params.BackgroundPixel != nil {
		image_params.background_pixel[0] = C.uint8_t(params.BackgroundPixel[0])
		image_params.background_pixel[1] = C.uint8_t(params.BackgroundPixel[1])
		image_params.background_pixel[2] = C.uint8_t(params.BackgroundPixel[2])
		image_params.background_pixel[3] = C.uint8_t(params.BackgroundPixel[3])
		image_params.flags |= C.PT_IMAGE_BACKGROUND_PIXEL
	}

	return image_params
}
