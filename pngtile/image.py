"""
    Image handling.
"""


from werkzeug import Request, Response, exceptions
from werkzeug.utils import html

import pngtile.tile
from pngtile.application import BaseApplication
import pypngtile

import json

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

    def __init__ (self, **opts):
        BaseApplication.__init__(self, **opts)

    def render_image (self, request, image, name):
        """
            request:    werkzeug.Request
            image:      pypngtile.Image
            name:       request path for .png image
        """

        image_info = image.info()

        map_config = dict(
            tile_url        = 'http://zovoweix.qmsk.net:8080/{name}?x={x}&y={y}&zoom={z}',
            tile_name       = name,

            tile_size       = pngtile.tile.TILE_SIZE,
            tile_zoom       = pngtile.tile.MAX_ZOOM,
            
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

    def handle (self, request):
        """
            Handle request for an image
        """
         
        try:
            image, name = self.get_image(request.path)
        except pypngtile.Error as error:
            raise exceptions.BadRequest(str(error))

        html = self.render_image(request, image, name)

        return Response(html, content_type="text/html")


