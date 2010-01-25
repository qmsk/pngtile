"""
    Our WSGI web interface, which can serve the JS UI and any .png tiles via HTTP.
"""

from werkzeug import Request, Response, responder
from werkzeug import exceptions
import os.path, os

import pypngtile as pt

## Settings
# path to images
DATA_ROOT = os.environ.get("PNGTILE_DATA_PATH") or os.path.abspath('data/')

# only open each image once
IMAGE_CACHE = {}

# width of a tile
TILE_WIDTH = 256
TILE_HEIGHT = 256

# max. output resolution to allow
MAX_PIXELS = 1920 * 1200

def dir_url (prefix, name, item) :
    """
        Join together an absolute URL prefix, an optional directory name, and a directory item
    """

    url = prefix

    if name :
        url = os.path.join(url, name)

    url = os.path.join(url, item)

    return url

def dir_list (dir_path) :
    """
        Yield a series of directory items to show for the given dir
    """

    # link to parent
    yield '..'

    for item in os.listdir(dir_path) :
        path = os.path.join(dir_path, item)

        # skip dotfiles
        if item.startswith('.') :
            continue
        
        # show dirs
        if os.path.isdir(path) :
            yield item

        # examine ext
        base, ext = os.path.splitext(path)

        # show .png files with a .cache file
        if ext == '.png' and os.path.exists(base + '.cache') :
            yield item

### Render HTML data
def render_dir (req, name, path) :
    """
        Directory index
    """

    prefix = os.path.dirname(req.script_root).rstrip('/')
    script_prefix = req.script_root
    name = name.rstrip('/')

    return """\
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
    <head>
        <title>Index of %(dir)s</title>
        <link rel="Stylesheet" type="text/css" href="%(prefix)s/static/style.css">
    </head>
    <body>
        <h1>Index of %(dir)s</h1>

        <ul>
%(listing)s
        </ul>
    </body>
</html>""" % dict(
        prefix          = prefix,
        dir             = '/' + name,
        
        listing         = "\n".join(
            # <li> link
            """<li><a href="%(url)s">%(name)s</a></li>""" % dict(
                # URL to dir
                url         = dir_url(script_prefix, name, item),

                # item name
                name        = item,
            ) for item in dir_list(path)
        ),
    )

def render_img_viewport (req, name, image) :
    """
        HTML for image
    """
    
    # a little slow, but not so bad - two stats(), heh
    info = image.info()
    img_width, img_height = info['img_width'], info['img_height']

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
                <div class="overlay">
                    <input type="button" id="btn-zoom-in" value="Zoom In" />
                    <input type="button" id="btn-zoom-out" value="Zoom Out" />
                    <a class="link" id="lnk-image" href="#"></a>
                </div>

                <div class="substrate"></div>

                <div class="background">
                    Loading...
                </div>
            </div>
        </div>

        <script type="text/javascript">
            var tile_source = new Source("%(tile_url)s", %(tile_width)d, %(tile_height)d, -4, 0, %(img_width)d, %(img_height)d);
            var main = new Viewport(tile_source, "viewport");
        </script>
    </body>
</html>""" % dict(
        title           = name,
        prefix          = os.path.dirname(req.script_root).rstrip('/'),
        tile_url        = req.url,

        tile_width      = TILE_WIDTH,
        tile_height     = TILE_HEIGHT,

        img_width       = img_width,
        img_height      = img_height,
    )

def scale_by_zoom (val, zoom) :
    """
        Scale coordinates by zoom factor
    """

    if zoom > 0 :
        return val << zoom

    elif zoom > 0 :
        return val >> -zoom

    else :
        return val

### Render PNG Data
def render_img_tile (image, x, y, zoom, width=TILE_WIDTH, height=TILE_HEIGHT) :
    """
        Render given tile, returning PNG data
    """

    return image.tile_mem(
        width, height, 
        scale_by_zoom(x, -zoom), scale_by_zoom(y, -zoom), 
        zoom
    )

def render_img_region (image, cx, cy, zoom, width, height) :
    """
        Render arbitrary tile, returning PNG data
    """

    x = scale_by_zoom(cx - width / 2, -zoom)
    y = scale_by_zoom(cy - height / 2, -zoom)

    # safely limit
    if width * height > MAX_PIXELS :
        raise exceptions.Forbidden("Image too large: %d * %d > %d" % (width, height, MAX_PIXELS))

    return image.tile_mem(
        width, height,
        x, y,
        zoom
    )


### Manipulate request data
def get_req_path (req) :
    """
        Returns the name and path requested
    """

    # path to image
    image_name = req.path.lstrip('/')
    
    # build absolute path
    image_path = os.path.abspath(os.path.join(DATA_ROOT, image_name))

    # ensure the path points inside the data root
    if not image_path.startswith(DATA_ROOT) :
        raise exceptions.NotFound(image_name)

    return image_name, image_path

def get_image (name, path) :
    """
        Gets an Image object from the cache, ensuring that it is cached
    """

    # get Image object
    if path in IMAGE_CACHE :
        # get from cache
        image = IMAGE_CACHE[path]

    else :
        # open
        image = pt.Image(path)

        # check
        if image.status() not in (pt.CACHE_FRESH, pt.CACHE_STALE) :
            raise exceptions.InternalServerError("Image cache not available: %s" % name)

        # load
        image.open()

        # cache
        IMAGE_CACHE[path] = image
    
    return image



### Handle request
def handle_dir (req, name, path) :
    """
        Handle request for a directory
    """

    return Response(render_dir(req, name, path), content_type="text/html")



def handle_img_viewport (req, image, name) :
    """
        Handle request for image viewport
    """

    # viewport
    return Response(render_img_viewport(req, name, image), content_type="text/html")


def handle_img_region (req, image) :
    """
        Handle request for an image region
    """

    # specific image
    width = int(req.args['w'])
    height = int(req.args['h'])
    cx = int(req.args['cx'])
    cy = int(req.args['cy'])
    zoom = int(req.args.get('zl', "0"))

    # yay full render
    return Response(render_img_region(image, cx, cy, zoom, width, height), content_type="image/png")


def handle_img_tile (req, image) :
    """
        Handle request for image tile
    """

    # tile
    x = int(req.args['x'])
    y = int(req.args['y'])
    zoom = int(req.args.get('zl', "0"))
        
    # yay render
    return Response(render_img_tile(image, x, y, zoom), content_type="image/png")

## Dispatch req to handle_img_*
def handle_img (req, name, path) :
    """
        Handle request for an image
    """

    # get image object
    image = get_image(name, path)

    # what view?
    if not req.args :
        return handle_img_viewport(req, image, name)

    elif 'w' in req.args and 'h' in req.args and 'cx' in req.args and 'cy' in req.args :
        return handle_img_region(req, image)

    elif 'x' in req.args and 'y' in req.args :
        return handle_img_tile(req, image)

    else :
        raise exceptions.BadRequest("Unknown args")



## Dispatch request to handle_*
def handle_req (req) :
    """
        Main request handler
    """
    
    # decode req
    name, path = get_req_path(req)

    # determine dir/image
    if os.path.isdir(path) :
        # directory
        return handle_dir(req, name, path)
    
    elif not os.path.exists(path) :
        # no such file
        raise exceptions.NotFound(name)

    elif not name or not name.endswith('.png') :
        # invalid file
        raise exceptions.BadRequest("Not a PNG file")
    
    else :
        # image
        return handle_img(req, name, path)




@responder
def application (env, start_response) :
    """
        Main WSGI entry point
    """

    req = Request(env, start_response)
    
    try :
        return handle_req(req)

    except exceptions.HTTPException, e :
        return e

