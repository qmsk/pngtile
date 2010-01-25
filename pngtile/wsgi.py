"""
    Our WSGI web interface, which can serve the JS UI and any .png tiles via HTTP.
"""

from werkzeug import Request, responder
from werkzeug import exceptions

from pngtile import handlers


@responder
def application (env, start_response) :
    """
        Main WSGI entry point.

        This is wrapped with werkzeug, so we can return a Response object
    """

    req = Request(env, start_response)
    
    try :
        return handlers.handle_req(req)

    except exceptions.HTTPException, e :
        return e

