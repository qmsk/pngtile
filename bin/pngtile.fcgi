import flup.server.fcgi

def main (app, bind=None) :
    """
        Run as a non-threaded single-process non-multiplexed FastCGI server
    """

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

if __name__ == '__main__' :
    import pngtile.wsgi

    main(pngtile.wsgi.application)

