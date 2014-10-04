"""
    Image handling.
"""


from werkzeug import Request, Response, exceptions
from werkzeug.utils import html, redirect
import werkzeug.urls

import pngtile.tile
import pngtile.application
import pypngtile

import json
import os, os.path

class ImageApplication (pngtile.application.PNGTileApplication):

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

    def __init__ (self, tiles_server=None, **opts):
        """
            http://.../ path to tileserver root
        """

        super(ImageApplication, self).__init__(**opts)
                
        self.tiles_server = tiles_server

    def tiles_url (self, name, **args):
        return werkzeug.urls.Href(self.tiles_server)(name, **args)

    def render_html (self, title, body, stylesheets=None, scripts=None, end=()):
        if stylesheets is None:
            stylesheets = self.STYLESHEETS

        if scripts is None:
            scripts = self.SCRIPTS

        return html.html(lang='en', *[
            html.head(
                html.title(title),
                *[
                    html.link(rel='stylesheet', href=href) for href in stylesheets
                ]
            ),
            html.body(
                *(body + tuple(
                    html.script(src=src) for src in scripts
                ) + end)
            ),
        ])


    def render_dir_breadcrumb (self, name):
        href = werkzeug.urls.Href('/')
        path = []

        yield html.li(html.a(href='/', *[u"Index"]))
        
        if name:
            for part in name.split('/'):
                path.append(part)

                yield html.li(html.a(href=href(*path), *[part]))

    def render_dir_item (self, name, type):
        if type:
            return html.a(href=name, *[name])
        else:
            dir = name + '/'
            return html.a(href=dir, *[dir])

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
                        html.ul(class_='list-group', *[
                            html.li(class_='list-group-item', *[
                                self.render_dir_item(name, type) for name, type in items
                            ]) for name, type in items
                        ]),
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
            tiles_url       = self.tiles_url(name),
            tiles_mtime     = image_info['cache_mtime'],

            tile_url        = '{tiles_url}?t={tiles_mtime}&x={x}&y={y}&zoom={z}',
            tile_size       = pngtile.tile.TILE_SIZE,
            tile_zoom       = pngtile.tile.MAX_ZOOM,
            
            # do NOT include a mtime in these urls.. we should always revalidate them
            image_url       = '{tiles_url}?w={w}&h={h}&x={x}&y={y}&zoom={z}',
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

        items = sorted(self.list(request.path))

        html = self.render_dir(request, name, items)

        return Response(html, content_type="text/html")

    def handle_image (self, request, name, path):
        """
            Generate Response for image request.
        """

        # backwards-compat redirect from frontend -> tile-server
        if all(attr in request.args for attr in ('cx', 'cy', 'w', 'h', 'zl')):
            return redirect(self.tiles_url(name,
                w       = request.args['w'],
                h       = request.args['h'],
                x       = request.args['cx'],
                y       = request.args['cy'],
                zoom    = request.args['zl'],
            ))

        image, name = self.open(request.path)

        html = self.render_image(request, image, name)

        return Response(html, content_type="text/html")

    def handle (self, request):
        """
            Handle request for an image
        """

        name, path, type = self.lookup(request.path)
        
        # determine handler
        if not type:
            return self.handle_dir(request, name, path)
        
        else:
            return self.handle_image(request, name, path)

    @Request.application
    def __call__ (self, request):
        """
            WSGI entry point.
        """

        try:
            return self.handle(request)

        except exceptions.HTTPException as error:
            return error

