
import pypngtile

import os.path
import threading

class Error (Exception):
    pass

class NotFound (KeyError):
    pass

class InvalidImage (Exception):
    pass

class UncachedImage (Exception):
    pass

class PNGTileStore (object):
    """
        Access pypngtile.Image's on a filesystem.
        
        Intended to be threadsafe for open()
    """

    IMAGE_TYPES = (
        'png',
    )

    def __init__ (self, image_root):
        if not os.path.isdir(image_root) :
            raise Error("Given image_root does not exist: {image_root}".format(image_root=image_root))

        self.image_root = os.path.abspath(image_root)
        
        # cache opened Images
        self.images = { }
        self.images_lock = threading.Lock()

    def lookup (self, url):
        """
            Lookup image by request path.

            Returns name, path, type. For dirs, type will be None.

            Raises Error, NotFound, InvalidImage

            Threadless.
        """

        if not os.path.isdir(self.image_root):
            raise Error("Server image_root has gone missing")
    
        # path to image
        name = url.lstrip('/')
        
        # build absolute path
        path = os.path.abspath(os.path.join(self.image_root, name))

        # ensure the path points inside the data root
        if not path.startswith(self.image_root):
            raise NotFound(name)
        
        if not os.path.exists(path):
            raise NotFound(name)
        
        # determine time
        if os.path.isdir(path):
            return name, path, None
        else:
            basename, type = path.rsplit('.', 1)

            if type not in self.IMAGE_TYPES:
                raise InvalidImage(name)
 
            return name, path, type

    def list (self, url):
        """
            Yield a series of valid sub-names for the given directory:
                foo.type
                foo/
            
            The yielded items should be sorted before use.
        """

        name, root, type = self.lookup(url)

        if type:
            raise InvalidImage(name)

        for name in os.listdir(root):
            path = os.path.join(root, name)

            # skip dotfiles
            if name.startswith('.'):
                continue
            
            # show dirs
            if os.path.isdir(path):
                if not os.access(path, os.R_OK):
                    # skip inaccessible dirs
                    continue

                yield name + '/'

            # examine ext
            if '.' in name:
                name_base, name_type = name.rsplit('.', 1)
            else:
                name_base = name
                name_type = None

            # show .png files with a .cache file
            if name_type in self.IMAGE_TYPES and os.path.exists(os.path.join(root, name_base + '.cache')):
                yield name

    def open (self, url):
        """
            Return Image object for given URL.

            Raises UncachedImage, pypngtile.Error

            Threadsafe.
        """

        name, path, type = self.lookup(url)

        if not type:
            raise InvalidImage(name)

        # get Image object
        with self.images_lock:
            image = self.images.get(path)

        if not image:
            # open
            image = pypngtile.Image(path)

            # check
            if image.status() not in (pypngtile.CACHE_FRESH, pypngtile.CACHE_STALE):
                raise UncachedImage(name)

            # load
            image.open()

            # cache
            with self.images_lock:
                # we don't really care if some other thread raced us on the same Image and opened it up concurrently...
                # this is just a cache
                self.images[path] = image
        
        return image, name
