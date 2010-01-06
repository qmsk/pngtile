"""
    Our WSGI web interface, which can serve the JS UI and any .png tiles via HTTP.
"""

from werkzeug import Request, Response, responder
from werkzeug import exceptions
import os.path

import pypngtile as pt

DATA_ROOT = os.path.abspath('data')

IMAGE_CACHE = {}

TILE_WIDTH = 256
TILE_HEIGHT = 256

def image_view (req, image_path, image) :
    image_name = os.path.basename(image_path)

    return """\
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
    <head>
        <title>%(title)s</title>
        <script src="%(prefix)s/static/prototype.js" type="text/javascript"></script>
        <script src="%(prefix)s/static/dragdrop.js" type="text/javascript"></script>
        <script src="%(prefix)s/static/builder.js" type="text/javascript"></script>
        <script src="%(prefix)s/static/tiles2.js" type="text/javascript"></script>
        <link rel="Stylesheet" type="text/css" href="%(prefix)s/static/style.css">
    </head>
    <body>
        <div id="wrapper">
            <div id="viewport" style="width: 100%%; height: 100%%">
                <div class="substrate"></div>
            </div>
        </div>

        <script type="text/javascript">
            var tile_source = new Source("%(tile_url)s", %(tile_width)d, %(tile_height)d, 0, 0);
            var main = new Viewport(tile_source, "viewport");
        </script>
    </body>
</html>""" % dict(
        title           = image_name,
        prefix          = os.path.dirname(req.script_root).rstrip('/'),
        tile_url        = req.url,

        tile_width      = TILE_WIDTH,
        tile_height     = TILE_HEIGHT,
    )

def render_tile (image, x, y) :
    return image.tile_mem(TILE_WIDTH, TILE_HEIGHT, x, y)

def handle_main (req) :
    # path to image
    image = req.path.lstrip('/')
    
    # check a .png filename was given
    if not image or not image.endswith('.png') :
        raise exceptions.BadRequest("no .png path given")
    
    # build absolute path
    image_path = os.path.abspath(os.path.join(DATA_ROOT, image))
    
    # ensure the path points inside the data root
    if not image_path.startswith(DATA_ROOT) :
        raise exceptions.NotFound(image)
    
    # get Image object
    if image_path in IMAGE_CACHE :
        # get from cache
        image = IMAGE_CACHE[image_path]

    else :
        # ensure exists
        if not os.path.exists(image_path) :
            raise exceptions.NotFound(image)

        # cache
        image = IMAGE_CACHE[image_path] = pt.Image(image_path)

    # what view?
    if not req.args :
        # viewport
        return Response(image_view(req, image_path, image), content_type="text/html")

    elif 'x' in req.args and 'y' in req.args :
        # tile
        x = int(req.args['x'])
        y = int(req.args['y'])
        
        # yay render
        return Response(render_tile(image, x, y), content_type="image/png")
   
    else :
        raise exceptions.BadRequest("Unknown args")
   

@responder
def application (env, start_response) :
    req = Request(env, start_response)
    
    try :
        return handle_main(req)

    except exceptions.HTTPException, e :
        return e

