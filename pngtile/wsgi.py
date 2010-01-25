"""
    Our WSGI web interface, which can serve the JS UI and any .png tiles via HTTP.
"""

from werkzeug import Request, Response, responder
from werkzeug import exceptions

import os.path, os
import pypngtile as pt

from pngtile import render

# path to images
DATA_ROOT = os.environ.get("PNGTILE_DATA_PATH") or os.path.abspath('data/')

# only open each image once
IMAGE_CACHE = {}


### Manipulate request data
def get_req_path (req) :
    """
        Returns the name and path requested
    """
    
    # check DATA_ROOT exists..
    if not os.path.isdir(DATA_ROOT) :
        raise exceptions.InternalServerError("Missing DATA_ROOT")

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
    
    prefix = os.path.dirname(req.script_root).rstrip('/')

    return Response(render.dir_html(prefix, name, path), content_type="text/html")



def handle_img_viewport (req, image, name) :
    """
        Handle request for image viewport
    """
    
    prefix = os.path.dirname(req.script_root).rstrip('/')

    # viewport
    return Response(render.img_html(prefix, name, image), content_type="text/html")


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
    
    try :
        # yay full render
        return Response(render.img_png_region(image, cx, cy, zoom, width, height), content_type="image/png")

    except ValueError, ex :
        # too large
        raise exceptions.Forbidden(str(ex))


def handle_img_tile (req, image) :
    """
        Handle request for image tile
    """

    # tile
    x = int(req.args['x'])
    y = int(req.args['y'])
    zoom = int(req.args.get('zl', "0"))
        
    # yay render
    return Response(render.img_png_tile(image, x, y, zoom), content_type="image/png")

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

