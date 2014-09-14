"""
    Raw tile handling.
"""

from pngtile.application import BaseApplication
from werkzeug import Request, Response, exceptions

import pypngtile

## Coordinates
# width of a tile
TILE_WIDTH = 256
TILE_HEIGHT = 256

# max. output resolution to allow
MAX_PIXELS = 1920 * 1200

def scale (val, zoom):
    """
        Scale dimension by zoom factor

        zl > 0 -> bigger
        zl < 0 -> smaller
    """

    if zoom > 0:
        return val << zoom

    elif zoom < 0:
        return val >> -zoom

    else:
        return val

def scale_center (val, dim, zoom):
    """
        Scale value about center by zoom.
    """

    return scale(val - dim / 2, zoom)

class TileApplication (BaseApplication):
    def __init__ (self, **opts):
        BaseApplication.__init__(self, **opts)
        
    def render_region (self, request, image):
        """
            Handle request for an image region
        """

        width = int(request.args['w'])
        height = int(request.args['h'])
        x = int(request.args['x'])
        y = int(request.args['y'])
        zoom = int(request.args.get('zoom', "0"))

        # safely limit
        if width * height > MAX_PIXELS:
            raise exceptions.BadRequest("Image size: %d * %d > %d" % (width, height, MAX_PIXELS))

        x = scale(x, zoom)
        y = scale(y, zoom)
        
        try:
            return image.tile_mem(width, height, x, y, zoom)

        except pypngtile.Error as error:
            raise exceptions.BadRequest(str(error))
        
    def render_tile (self, request, image):
        """
            Handle request for image tile
        """

        width = TILE_WIDTH
        height = TILE_HEIGHT
        row = int(request.args['row'])
        col = int(request.args['col'])
        zoom = int(request.args.get('zoom', "0"))

        x = scale(row * width, zoom)
        y = scale(col * height, zoom)

        try:
            return image.tile_mem(width, height, x, y, zoom)

        except pypngtile.Error as error:
            raise exceptions.BadRequest(str(error))

    def handle (self, request):
        """
            Handle request for an image
        """
        
        try:
            image, name = self.get_image(request.path)
        except pypngtile.Error as error:
            raise exceptions.BadRequest(str(error))

        if 'w' in request.args and 'h' in request.args and 'x' in request.args and 'y' in request.args:
            png = self.render_region(request, image)

        elif 'row' in request.args and 'col' in request.args:
            png = self.render_tile(request, image)

        else:
            raise exceptions.BadRequest("Unknown args")

        return Response(png, content_type='image/png')
