"""
    Raw tile handling.
"""

import os.path

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

class Application (object):
    def __init__ (self, image_root):
        if not os.path.isdir(image_root) :
            raise Exception("Given image_root does not exist: {image_root}".format(image_root=image_root))

        self.image_root = os.path.abspath(image_root)

        self.image_cache = { }

    def lookup_image (self, url):
        """
            Lookup image by request path.

            Returns image_name, image_path.
        """

        if not os.path.isdir(self.image_root):
            raise exceptions.InternalServerError("Server image_root has gone missing")
    
        # path to image
        name = url.lstrip('/')
        
        # build absolute path
        path = os.path.abspath(os.path.join(self.image_root, name))

        # ensure the path points inside the data root
        if not path.startswith(self.image_root):
            raise exceptions.NotFound(name)

        return name, path

    def get_image (self, url):
        """
            Return Image object.
        """

        name, path = self.lookup_image(url)

        # get Image object
        image = self.image_cache.get(path)

        if not image:
            # open
            image = pypngtile.Image(path)

            # check
            if image.status() not in (pypngtile.CACHE_FRESH, pypngtile.CACHE_STALE):
                raise exceptions.InternalServerError("Image cache not available: {name}".format(name=name))

            # load
            image.open()

            # cache
            self.image_cache[path] = image
        
        return image

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
            image = self.get_image(request.path)
        except pypngtile.Error as error:
            raise exceptions.BadRequest(str(error))

        if 'w' in request.args and 'h' in request.args and 'x' in request.args and 'y' in request.args:
            png = self.render_region(request, image)

        elif 'row' in request.args and 'col' in request.args:
            png = self.render_tile(request, image)

        else:
            raise exceptions.BadRequest("Unknown args")

        return Response(png, content_type='image/png')

    @Request.application
    def __call__ (self, request):
        """
            WSGI entry point.
        """

        try:
            return self.handle(request)

        except exceptions.HTTPException as error:
            return error

