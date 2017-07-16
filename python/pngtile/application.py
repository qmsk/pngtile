from werkzeug import Request, Response, exceptions

import pypngtile
import pngtile.store

class PNGTileApplication (pngtile.store.PNGTileStore):
    """
        Web application with a PNGTileStore.
    """
    
    def list (self, url):
        """
            Yield names by request.url.

            Raises HTTPException
        """

        try:
            return super(PNGTileApplication, self).list(url)
        except pngtile.store.NotFound as error:
            raise exceptions.NotFound(str(error))
        except pngtile.store.InvalidImage as error:
            raise exceptions.BadRequest(str(error))
   
    def lookup (self, url):
        """
            Lookup neme, path, type by request.url.

            Raises HTTPException
        """

        try:
            return super(PNGTileApplication, self).lookup(url)
        except pngtile.store.Error as error:
            raise exceptions.InternalServerError(str(error))
        except pngtile.store.NotFound as error:
            raise exceptions.NotFound(str(error))
        except pngtile.store.InvalidImage  as error:
            raise exceptions.BadRequest(str(error))

    def open (self, url):
        """
            Return Image, name by request.url

            Raises HTTPException.
        """

        try:
            return super(PNGTileApplication, self).open(url)

        except pypngtile.Error as error:
            raise exceptions.InternalServerError(str(error))

        except pngtile.store.Error as error:
            raise exceptions.InternalServerError(str(error))

        except pngtile.store.NotFound as error:
            raise exceptions.NotFound(str(error))

        except pngtile.store.InvalidImage  as error:
            raise exceptions.BadRequest(str(error))

        except pngtile.store.UncachedImage as error:
            raise exceptions.InternalServerError("Requested image has not yet been cached: {image}".format(image=error))

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

