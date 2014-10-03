"""
    Image handling.
"""


from werkzeug import Request, Response, exceptions
from werkzeug.utils import html, redirect

import pngtile.tile
from pngtile.application import url, BaseApplication
import pypngtile

import json
import os, os.path

def dir_list (root):
    """
        Yield a series of directory items to show for the given dir
    """

    for name in os.listdir(root):
        path = os.path.join(root, name)

        # skip dotfiles
        if name.startswith('.'):
            continue
        
        # show dirs
        if os.path.isdir(path):
            if not os.access(path, os.R_OK):
                # skip inaccessible dirs
                continue

            yield name + '/'

        # examine ext
        if '.' in name:
            name_base, name_type = name.rsplit('.', 1)
        else:
            name_base = name
            name_type = None

        # show .png files with a .cache file
        if name_type in ImageApplication.IMAGE_TYPES and os.path.exists(os.path.join(root, name_base + '.cache')):
            yield name

class ImageApplication (BaseApplication):

    STYLESHEETS = (
        'https://maxcdn.bootstrapcdn.com/bootstrap/3.2.0/css/bootstrap.min.css',
        'https://maxcdn.bootstrapcdn.com/bootstrap/3.2.0/css/bootstrap-theme.min.css',
        'http://cdn.leafletjs.com/leaflet-0.7.3/leaflet.css',
        '/static/pngtile/map.css',
    )

    SCRIPTS = (
        'https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js',
        'https://maxcdn.bootstrapcdn.com/bootstrap/3.2.0/js/bootstrap.min.js',
        'http://cdn.leafletjs.com/leaflet-0.7.3/leaflet.js',
        '/static/pngtile/map.js',
    )

    def __init__ (self, tile_server=None, **opts):
        """
            http://.../ path to tileserver root
        """

        BaseApplication.__init__(self, **opts)
                
        self.tile_server = tile_server

    def render_dir_breadcrumb (self, name):
        path = []

        yield html.li(html.a(href='/', *[u"Index"]))
        
        if name:
            for part in name.split('/'):
                path.append(part)

                yield html.li(html.a(href=url('/', *path), *[part]))

    def render_dir (self, request, name, items):
        """
            request:    werkzeug.Request
            name:       /.../... url to dir
            items:      [...] items in dir
        """
        
        if name:
            title = name
        else:
            title = "Index"

        return self.render_html(
            title       = name,
            body        = (
                html.div(class_='container', *[
                    html.h1(title),
                    html.div(*[
                        html.ol(class_='breadcrumb', *self.render_dir_breadcrumb(name)),
                    ]),
                    html.div(class_='list', *[
                        html.ul(class_='list-group', *[html.li(class_='list-group-item', *[
                                html.a(href=item, *[item])
                            ]) for item in items]
                        ),
                    ]),
                ]),
            ),
        )

    def render_image (self, request, image, name):
        """
            request:    werkzeug.Request
            image:      pypngtile.Image
            name:       request path for .../.../*.png image
        """

        image_info = image.info()

        map_config = dict(
            tile_url        = '{server}/{name}?x={x}&y={y}&zoom={z}',
            tile_server     = self.tile_server.rstrip('/'),
            tile_name       = name,

            tile_size       = pngtile.tile.TILE_SIZE,
            tile_zoom       = pngtile.tile.MAX_ZOOM,
            
            image_url       = '{server}/{name}?w={w}&h={h}&x={x}&y={y}&zoom={z}',
            image_server    = self.tile_server.rstrip('/'),
            image_name      = name,
            image_width     = image_info['img_width'],
            image_height    = image_info['img_height'],
        )

        return self.render_html(
            title       = name,
            body        = (
                html.div(id='wrapper', *[
                    html.div(id='map')
                ]),
            ),
            end         = (
                html.script("""$(function() {{ map_init({map_config}); }});""".format(map_config=json.dumps(map_config))),
            ),
        )

    def handle_dir (self, request, name, path):
        """
            Generate response for directory listing.
        """

        if not request.path.endswith('/'):
            # we generate HTML with relative links
            return redirect(request.path + '/')

        items = sorted(dir_list(path))

        html = self.render_dir(request, name, items)

        return Response(html, content_type="text/html")

    def handle_image (self, request, name, path):
        """
            Generate Response for image request.
        """

        # backwards-compat redirect from frontend -> tile-server
        if all(attr in request.args for attr in ('cx', 'cy', 'w', 'h', 'zl')):
            return redirect(url(self.tile_server, name,
                w       = request.args['w'],
                h       = request.args['h'],
                x       = request.args['cx'],
                y       = request.args['cy'],
                zoom    = request.args['zl'],
            ))

        try:
            image, name = self.get_image(request.path)
        except pypngtile.Error as error:
            raise exceptions.BadRequest(str(error))

        html = self.render_image(request, image, name)

        return Response(html, content_type="text/html")

    def handle (self, request):
        """
            Handle request for an image
        """

        name, path, type = self.lookup_path(request.path)
        
        # determine handler
        if not type:
            return self.handle_dir(request, name, path)
        
        else:
            return self.handle_image(request, name, path)

