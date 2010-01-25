"""
    A tornado-based HTTP app
"""

import tornado.web
import tornado.httpserver
import tornado.wsgi
import werkzeug

from pngtile import handlers

class MainHandler (tornado.web.RequestHandler) :
    """
        Main handler for the / URL, pass off requests to werkzeug-based handlers...
    """

    def build_environ (self, path) :
        """
            Yield a series of (key, value) pairs suitable for use with WSGI
        """

        request = self.request

        hostport = request.host.split(":")

        if len(hostport) == 2:
            host = hostport[0]
            port = int(hostport[1])
        else:
            host = request.host
            port = 443 if request.protocol == "https" else 80

        yield "REQUEST_METHOD", request.method
        yield "SCRIPT_NAME", ""
        yield "PATH_INFO", path
        yield "QUERY_STRING", request.query
        yield "SERVER_NAME", host
        yield "SERVER_PORT", port

        yield "wsgi.version", (1, 0)
        yield "wsgi.url_scheme", request.protocol
        
        yield "CONTENT_TYPE", request.headers.get("Content-Type")
        yield "CONTENT_LENGTH", request.headers.get("Content-Length")

        for key, value in request.headers.iteritems():
            yield "HTTP_" + key.replace("-", "_").upper(), value
    
    def get (self, path) :
        environ = dict(self.build_environ(path))

        # build Request
        request = werkzeug.Request(environ)

        # handle
        try :
            response = handlers.handle_req(request)

        except werkzeug.exceptions.HTTPException, ex :
            response = ex

        # return
        def start_response (_status, _headers) :
            status = int(_status.split()[0])
            headers = _headers

            self.set_status(status)

            for name, value in headers :
                self.set_header(name, value)
        
        # invoke Response
        data = response(environ, start_response)

        # output data
        for chunk in data :
            self.write(chunk)
        
def build_app () :
    return tornado.web.Application([
        # static, from $CWD/static/
        (r"/static/(.*)",           tornado.web.StaticFileHandler,  dict(path = "static/")),
        
        # dir listings, image html, PNG tiles
        (r"(/.*)",     MainHandler),
    ])  

def build_httpserver (app, port) :
    server = tornado.httpserver.HTTPServer(app)
    server.listen(port)

    return server

def main (port=8000) :
    """
        Build the app, http server and run the main loop
    """

    import logging

    logging.basicConfig(level=logging.DEBUG)

    app = build_app()
    server = build_httpserver(app, port)

    tornado.ioloop.IOLoop.instance().start()


