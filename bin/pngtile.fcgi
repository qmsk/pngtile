#!/usr/bin/env python2.5

import flup.server.fcgi

import memcache

def run_fastcgi (app, bind=None) :
    # create WSGIServer
    server = flup.server.fcgi.WSGIServer(app, 
        # try to supress threading
        multithreaded=False, 
        multiprocess=False, 
        multiplexed=False,
        
        # specify the bind() address
        bindAddress=bind,

        # leave as defaults for now
        umask=None,

        # XXX: non-debug mode?
        debug=True,
    )
    
    # run... threads :(
    server.run()

def main (bind=None) :
    """
        Run as a non-threaded single-process non-multiplexed FastCGI server
    """

    # open cache
    cache = memcache.Client(['localhost:11211'])
    
    # build app
    app = pngtile.wsgi.WSGIApplication(cache)

    # server
    run_fastcgi(app, bind)

if __name__ == '__main__' :
    import pngtile.wsgi

    main()

