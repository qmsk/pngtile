from werkzeug import Request, Response, exceptions
from werkzeug.utils import html

import pypngtile

import os.path

class BaseApplication (object):
    IMAGE_TYPES = (
        'png',
    )

    def __init__ (self, image_root):
        if not os.path.isdir(image_root) :
            raise Exception("Given image_root does not exist: {image_root}".format(image_root=image_root))

        self.image_root = os.path.abspath(image_root)

        self.image_cache = { }

    def lookup_path (self, url):
        """
            Lookup image by request path.

            Returns name, path.
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
        
        if not os.path.exists(path):
            raise exceptions.NotFound(name)

        return name, path

    def get_image (self, url):
        """
            Return Image object.
        """

        name, path = self.lookup_path(url)

        if os.path.isdir(path):
            raise exceptions.BadRequest("Is a directory: {name}".format(name=name))

        basename, file_type = path.rsplit('.', 1)

        if file_type not in self.IMAGE_TYPES:
            raise exceptions.BadRequest("Not a supported image: {name}: {type}".format(name=name, type=file_type))

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
        
        return image, name

    def handle (self, request):
        """
            Handle request for an image
        """

        raise NotImplementedError()

    @Request.application
    def __call__ (self, request):
        """
            WSGI entry point.
        """

        try:
            return self.handle(request)

        except exceptions.HTTPException as error:
            return error

    STYLESHEETS = ( )
    SCRIPTS = ( )

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


