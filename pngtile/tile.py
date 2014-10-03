"""
    Raw tile handling.
"""

from pngtile.application import BaseApplication, url
from werkzeug import Request, Response, exceptions
from werkzeug.utils import redirect

import pypngtile

## Coordinates
# width of a tile
TILE_SIZE = 256

# maximum zoom out
MAX_ZOOM = 4

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

    return scale(val, zoom) - dim / 2

class TileApplication (BaseApplication):
    def __init__ (self, image_server, **opts):
        """
            image_server:       http://.../ url to image-server frontend
        """

        BaseApplication.__init__(self, **opts)

        self.image_server = image_server
        
    def render_region (self, request, image):
        """
            Handle request for an image region
        """

        width = int(request.args['w'])
        height = int(request.args['h'])
        x = int(request.args['x'])
        y = int(request.args['y'])
        zoom = int(request.args.get('zoom', "0"))

        # safety limit
        if width * height > MAX_PIXELS:
            raise exceptions.BadRequest("Image size: %d * %d > %d" % (width, height, MAX_PIXELS))

        if zoom > MAX_ZOOM:
            raise exceptions.BadRequest("Image zoom: %d > %d" % (zoom, MAX_ZOOM))

        x = scale_center(x, width, zoom)
        y = scale_center(y, height, zoom)
        
        try:
            return image.tile_mem(width, height, x, y, zoom)

        except pypngtile.Error as error:
            raise exceptions.BadRequest(str(error))
        
    def render_tile (self, request, image):
        """
            Handle request for image tile
        """

        width = TILE_SIZE
        height = TILE_SIZE
        x = int(request.args['x'])
        y = int(request.args['y'])
        zoom = int(request.args.get('zoom', "0"))
        
        if zoom > MAX_ZOOM:
            raise exceptions.BadRequest("Image zoom: %d > %d" % (zoom, MAX_ZOOM))

        x = scale(x * width, zoom)
        y = scale(y * height, zoom)

        try:
            return image.tile_mem(width, height, x, y, zoom)

        except pypngtile.Error as error:
            raise exceptions.BadRequest(str(error))

    def handle_dir (self, request, name, path):
        """
            Redirect to the image frontend for a non-tile request.
        """

        if not name.endswith('/'):
            # avoid an additional redirect
            name += '/'

        return redirect(url(self.image_server, name))

    def handle_image (self, request, name, path):
        """
            Redirect to the image frontend for a non-tile request.
        """

        return redirect(url(self.image_server, name))

    def handle_region (self, request):
        """
            Return image/png for given region.
        """

        try:
            image, name = self.get_image(request.path)
        except pypngtile.Error as error:
            raise exceptions.BadRequest(str(error))
 
        png = self.render_region(request, image)
        
        return Response(png, content_type='image/png')

    def handle_tile (self, request):
        """
            Return image/png for given tile.
        """

        try:
            image, name = self.get_image(request.path)
        except pypngtile.Error as error:
            raise exceptions.BadRequest(str(error))
            
        png = self.render_tile(request, image)
        
        return Response(png, content_type='image/png')

    def handle (self, request):
        """
            Handle request for an image
        """

        name, path, type = self.lookup_path(request.path)
        
        # determine handler
        if not type:
            return self.handle_dir(request, name, path)

        elif not request.args:
            return self.handle_image(request, name, path)

        elif 'w' in request.args and 'h' in request.args and 'x' in request.args and 'y' in request.args:
            return self.handle_region(request)

        elif 'x' in request.args and 'y' in request.args:
            return self.handle_tile(request)

        else:
            raise exceptions.BadRequest("Unknown args")

