"""
    Rendering output
"""

import os, os.path

## Settings
# width of a tile
TILE_WIDTH = 256
TILE_HEIGHT = 256

# max. output resolution to allow
MAX_PIXELS = 1920 * 1200

def dir_url (prefix, name, item) :
    """
        Join together an absolute URL prefix, an optional directory name, and a directory item
    """

    url = prefix

    if name :
        url += '/' + name
    
    url += '/' + item

    return url

def dir_list (dir_path) :
    """
        Yield a series of directory items to show for the given dir
    """

    # link to parent
    yield '..'

    for item in os.listdir(dir_path) :
        path = os.path.join(dir_path, item)

        # skip dotfiles
        if item.startswith('.') :
            continue
        
        # show dirs
        if os.path.isdir(path) :
            yield item

        # examine ext
        base, ext = os.path.splitext(path)

        # show .png files with a .cache file
        if ext == '.png' and os.path.exists(base + '.cache') :
            yield item


### Render HTML data
def dir_html (prefix, name, path) :
    """
        Directory index
    """

    name = name.rstrip('/')

    return """\
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
    <head>
        <title>Index of %(dir)s</title>
        <link rel="Stylesheet" type="text/css" href="%(prefix)s/static/style.css">
    </head>
    <body>
        <h1>Index of %(dir)s</h1>

        <ul>
%(listing)s
        </ul>
    </body>
</html>""" % dict(
        prefix          = prefix,
        dir             = '/' + name,
        
        listing         = "\n".join(
            # <li> link
            """<li><a href="%(url)s">%(name)s</a></li>""" % dict(
                # URL to dir
                url         = dir_url(prefix, name, item),

                # item name
                name        = item,
            ) for item in dir_list(path)
        ),
    )

def img_html (prefix, name, image) :
    """
        HTML for image
    """
    
    # a little slow, but not so bad - two stats(), heh
    info = image.info()
    img_width, img_height = info['img_width'], info['img_height']

    return """\
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
    <head>
        <title>%(title)s</title>
        <script src="%(prefix)s/static/prototype.js" type="text/javascript"></script>
        <script src="%(prefix)s/static/dragdrop.js" type="text/javascript"></script>
        <script src="%(prefix)s/static/builder.js" type="text/javascript"></script>
        <script src="%(prefix)s/static/tiles2.js" type="text/javascript"></script>
        <link rel="Stylesheet" type="text/css" href="%(prefix)s/static/style.css">
    </head>
    <body>
        <div id="wrapper">
            <div id="viewport" style="width: 100%%; height: 100%%">
                <div class="overlay">
                    <input type="button" id="btn-zoom-in" value="Zoom In" />
                    <input type="button" id="btn-zoom-out" value="Zoom Out" />
                    <a class="link" id="lnk-image" href="#"></a>
                </div>

                <div class="substrate"></div>

                <div class="background">
                    Loading...
                </div>
            </div>
        </div>

        <script type="text/javascript">
            var tile_source = new Source("%(tile_url)s", %(tile_width)d, %(tile_height)d, 0, 4, %(img_width)d, %(img_height)d);
            var main = new Viewport(tile_source, "viewport");
        </script>
    </body>
</html>""" % dict(
        title           = name,
        prefix          = prefix,
        tile_url        = prefix + '/' + name,

        tile_width      = TILE_WIDTH,
        tile_height     = TILE_HEIGHT,

        img_width       = img_width,
        img_height      = img_height,
    )


# threshold to cache images on - only images with a source data region *larger* than this are cached
CACHE_THRESHOLD = 512 * 512

def scale_by_zoom (val, zoom) :
    """
        Scale dimension by zoom factor

        zl > 0 -> bigger
        zl < 0 -> smaller
    """

    if zoom > 0 :
        return val << zoom

    elif zoom > 0 :
        return val >> -zoom

    else :
        return val

### Image caching
def check_cache_threshold (width, height, zl) :
    """
        Checks if a tile with the given dimensions should be cached
    """

    return (scale_by_zoom(width, zl) * scale_by_zoom(height, zl)) > CACHE_THRESHOLD

def render_raw (image, width, height, x, y, zl) :
    """
        Render and return tile
    """

    return image.tile_mem(
            width, height,
            x, y, zl
    )

def render_cache (cache, image, width, height, x, y, zl) :
    """
        Perform a cached render of the given tile
    """
    
    if cache :
        # cache key
        key = "tl_%d:%d_%d:%d:%d_%s" % (x, y, width, height, zl, image.path)
        
        # lookup
        data = cache.get(key)

    else :
        # oops, no cache
        data = None

    if not data :
        # cache miss, render
        data = render_raw(image, width, height, x, y, zl)
        
        if cache :
            # store
            cache.add(key, data)
    
    # ok
    return data

### Render PNG Data
def img_png_tile (image, x, y, zoom, cache) :
    """
        Render given tile, returning PNG data
    """

    # remap coordinates by zoom
    x = scale_by_zoom(x, zoom)
    y = scale_by_zoom(y, zoom)

    # do we want to cache this?
    if check_cache_threshold(TILE_WIDTH, TILE_HEIGHT, zoom) :
        # go via the cache
        return render_cache(cache, image, TILE_WIDTH, TILE_HEIGHT, x, y, zoom)

    else :
        # just go raw
        return render_raw(image, TILE_WIDTH, TILE_HEIGHT, x, y, zoom)

def img_png_region (image, cx, cy, zoom, width, height, cache) :
    """
        Render arbitrary tile, returning PNG data
    """

    x = scale_by_zoom(cx - width / 2, zoom)
    y = scale_by_zoom(cy - height / 2, zoom)

    # safely limit
    if width * height > MAX_PIXELS :
        raise ValueError("Image size: %d * %d > %d" % (width, height, MAX_PIXELS))
    
    # these images are always cached
    return render_cache(cache, image, width, height, x, y, zoom)

