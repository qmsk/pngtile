"""
    Our WSGI web interface, which can serve the JS UI and any .png tiles via HTTP.
"""

from werkzeug import Request, responder
from werkzeug import exceptions

from pngtile import handlers


class WSGIApplication (object) :
    """
        Simple WSGI application invoking the werkzeug handlers
    """

    def __init__ (self, cache=None) :
        """
            Use given cache if any
        """

        self.cache = cache

    @responder
    def __call__ (self, env, start_response) :
        """
            Main WSGI entry point.

            This is wrapped with werkzeug, so we can return a Response object
        """

        req = Request(env, start_response)
        
        try :
            return handlers.handle_req(req, self.cache)

        except exceptions.HTTPException, e :
            return e

