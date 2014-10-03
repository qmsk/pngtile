"""
    Raw tile handling.
"""

from werkzeug import Request, Response, exceptions
from werkzeug.utils import redirect
import werkzeug.urls

import pngtile.application
import pypngtile

import datetime

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

class TileApplication (pngtile.application.PNGTileApplication):
    # age in seconds for caching a known-mtime image
    MAX_AGE = 7 * 24 * 60 * 60 # 1 week

    def __init__ (self, image_server, **opts):
        """
            image_server:       http://.../ url to image-server frontend
        """

        super(TileApplication, self).__init__(**opts)

        self.image_server = image_server
        
    def image_url (self, name):
        return werkzeug.urls.Href(self.image_server)(path)

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
            raise exceptions.InternalServerError(str(error))
        
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
            raise exceptions.InternalServerError(str(error))

    def handle_dir (self, request, name, path):
        """
            Redirect to the image frontend for a non-tile request.
        """

        if not name.endswith('/'):
            # avoid an additional redirect
            name += '/'

        return redirect(self.image_url(name))

    def handle_image (self, request, name, path):
        """
            Redirect to the image frontend for a non-tile request.
        """

        return redirect(self.image_url(name))

    def handle (self, request):
        """
            Handle request for an image
        """
        
        name, path, type = self.lookup(request.path)
        
        # determine handler
        if not type:
            return self.handle_dir(request, name, path)

        elif not request.args:
            return self.handle_image(request, name, path)

        elif 'w' in request.args and 'h' in request.args and 'x' in request.args and 'y' in request.args:
            render_func = self.render_region

        elif 'x' in request.args and 'y' in request.args:
            render_func = self.render_tile

        else:
            raise exceptions.BadRequest("Unknown args")
        
        # handle image
        image, name = self.open(request.path)

        # http caching
        mtime = image.cache_mtime()

        if 't' in request.args:
            try:
                ttime = datetime.datetime.utcfromtimestamp(int(request.args['t']))
            except ValueError:
                ttime = None
        else:
            ttime = None

        if request.if_modified_since and mtime == request.if_modified_since:
            return Response(status=304)
            
        # render
        png = render_func(request, image)
        
        # response
        response = Response(png, content_type='image/png')
        response.last_modified = mtime

        if not ttime:
            # cached item may change while url stays the same
            response.headers['Cache-Control'] = 'must-revalidate'

        elif ttime == mtime:
            # url will change if content changes
            response.headers['Cache-Control'] = 'max-age={max_age:d}'.format(max_age=self.MAX_AGE)

        else:
            # XXX: mismatch wtf
            response.headers['Cache-Control'] = 'must-revalidate'

        return response


