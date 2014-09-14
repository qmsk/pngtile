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
        '/static/pngtile/image.css',
    )

    SCRIPTS = (
        'https://ajax.googleapis.com/ajax/libs/jquery/1.11.1/jquery.min.js',
        'https://maxcdn.bootstrapcdn.com/bootstrap/3.2.0/js/bootstrap.min.js',
        'http://cdn.leafletjs.com/leaflet-0.7.3/leaflet.js',
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

        config = dict(
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
                html.script("""\
                    $(function() {{
                        var config = {config};

                        var bounds = [
                            [ 0, config.image_height ],
                            [ -config.image_width, 0 ]
                        ];
                        var center = [
                            -256, 512
                        ];
                        var zoom = config.tile_zoom;
                        
                        var map = L.map('map', {{
                            crs: L.CRS.Simple,
                            center: center,
                            zoom: zoom,
                            maxBounds: bounds
                        }});

                        L.tileLayer(config.tile_url, {{
                            name: config.tile_name,
                            minZoom: 0,
                            maxZoom: config.tile_zoom,
                            tileSize: config.tile_size,
                            continuousWorld: true,
                            noWrap: true,
                            zoomReverse: true,
                            bounds: bounds
                        }}).addTo(map);
                    }});
                """.format(config=json.dumps(config))),
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


