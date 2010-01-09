// A source of tile images of a specific width/height, zoom level range, and some other attributes

/**
 * A source of image tiles.
 *
 * The image source is expected to serve fixed-size tiles (tile_width × tile_height) of image data based on the 
 * x, y, zl URL query parameters. 
 *
 *  x, y        - the pixel coordinates of the top-left corner
 *                XXX: these are scaled from the image coordinates by the zoom level
 *
 *  zl          - the zoom level used, out < zl < in.
 *                The image pixels are scaled by powers-of-two, so a 256x256 tile at zl=-1 shows a 512x512 area of the
 *                1:1 image.
 */
var Source = Class.create({
    initialize: function (path, tile_width, tile_height, zoom_min, zoom_max, img_width, img_height) {
        this.path = path;
        this.tile_width = tile_width;
        this.tile_height = tile_height;
        this.zoom_min = zoom_min;
        this.zoom_max = zoom_max;
        this.img_width = img_width;
        this.img_height = img_height;

        this.refresh = false;
        this.opt_key = this.opt_value = null;
    },
    
    /**
     * Return an URL representing the tile at the given viewport (row, col) at the given zl.
     *
     * XXX: sw/sh = screen-width/height, used to choose an appropriate output size for dynamic image sources...
     */
    build_url: function (col, row, zl /*, sw, sh */) {
        // XXX: distribute tile requests across tile*.foo.bar
        if (0) {
            // two-bit hash (0-4) based on the (col, row)
            var hash = ( (col % 2) << 1 | (row % 2) ) + 1;
            
            // the subdomain to use
            var subdomain = "";
            
            subdomain = "tile" + hash + ".";
        }

        // the (x, y) co-ordinates
        var x = col * this.tile_width;
        var y = row * this.tile_height;

        var url = this.path + "?x=" + x + "&y=" + y + "&zl=" + zl; // + "&sw=" + sw + "&sh=" + sh;
        
        // refresh the tile each time it is loaded?
        if (this.refresh)
            url += "&ts=" + new Date().getTime();
        
        // XXX: additional parameters, not used
        if (this.opt_key && this.opt_value)
            url += "&" + this.opt_key + "=" + this.opt_value;

        return url;
    },

    // build an URL for a full image
    build_image_url: function (cx, cy, w, h, zl) {
        return (this.path + "?cx=" + cx + "&cy=" + cy + "&w=" + w + "&h=" + h + "&zl=" + zl);
    }
});

/**
 * Viewport implements the tiles-UI. It consists of a draggable substrate, which in turn consists of several
 * ZoomLayers, which then contain the actual tile images.
 *
 * Vars:
 *      scroll_x/y      - the visible pixel offset of the top-left corner
 */
var Viewport = Class.create({
    initialize: function (source, viewport_id) {
        this.source = source;
        
        // get a handle on the UI elements
        this.id = viewport_id;
        this.div = $(viewport_id);
        this.substrate = this.div.down("div.substrate");

        // make the substrate draggable
        this.draggable = new Draggable(this.substrate, {
            onStart: this.on_scroll_start.bind(this),
            onDrag: this.on_scroll_move.bind(this),
            onEnd: this.on_scroll_end.bind(this),

            zindex: false
        });

        // register event handlers for other UI functions
        Event.observe(this.substrate, "dblclick", this.on_dblclick.bindAsEventListener(this));
        Event.observe(this.substrate, "mousewheel", this.on_mousewheel.bindAsEventListener(this));
        Event.observe(this.substrate, "DOMMouseScroll", this.on_mousewheel.bindAsEventListener(this));     // mozilla
        Event.observe(document, "resize", this.on_resize.bindAsEventListener(this));
        
        // init zoom UI buttons
        this.btn_zoom_in = $("btn-zoom-in");
        this.btn_zoom_out = $("btn-zoom-out");

        if (this.btn_zoom_in)
            Event.observe(this.btn_zoom_in, "click", this.zoom_in.bindAsEventListener(this));
        
        if (this.btn_zoom_out)
            Event.observe(this.btn_zoom_out, "click", this.zoom_out.bindAsEventListener(this));
           
        // initial view location (centered)
        var cx = this.source.img_width / 2;
        var cy = this.source.img_height / 2;
        var zl = 0; // XXX: would need to scale x/y for this: (this.source.zoom_min + this.source.zoom_max) / 2;
        
        // from link?
        if (document.location.hash) {
            // parse x:y:z tuple
            var pt = document.location.hash.substr(1).split(":");
            
            // unpack    
            if (pt.length) cx = parseInt(pt.shift()) || cx;
            if (pt.length) cy = parseInt(pt.shift()) || cy;
            if (pt.length) zl = parseInt(pt.shift()) || zl;
        }

        // initialize zoom state to given zl
        this._init_zoom(zl);
        
        // initialize viewport size
        this.update_size();
        
        // initialize scroll offset
        this._init_scroll(cx, cy);
        
        // this comes after update_size, so that the initial update_size doesn't try and update the image_link,
        // since this only works once we have the zoom layers set up...
        this.image_link = $("lnk-image");
        this.update_image_link();

        // display tiles!
        this.update_tiles();
    },

/*
 * Initializers - only run once
 */
                    
    /** Initialize the zoom state to show the given zoom level */
    _init_zoom: function (zl) {
        // the stack of zoom levels
        this.zoom_layers = [];
        
        // populate the zoom-layers stack based on the number of zoom levels defined for the source
        for (var zoom_level = this.source.zoom_min; zoom_level <= this.source.zoom_max; zoom_level++) {
            var zoom_layer = new ZoomLayer(this.source, zoom_level);

            this.substrate.appendChild(zoom_layer.div);
            this.zoom_layers[zoom_level] = zoom_layer;
        }

        // is the new zoom level valid?
        if (!this.zoom_layers[zl])
            // XXX: nasty, revert to something else?
            return false;
        
        // set the zoom layyer
        this.zoom_layer = this.zoom_layers[zl];
        
        // enable it with initial z-index
        this.zoom_layer.enable(11);
        
        // init the UI accordingly
        this.update_zoom_ui();
    },

    /** Initialize the scroll state to show the given (scaled) centered coordinates */
    _init_scroll: function (cx, cy) {
        // scroll center
        this.scroll_center_to(cx, cy);
    },

    
/*
 * Handle input events
 */    

    /** Viewport resized */
    on_resize: function (ev) {
        this.update_size();
        this.update_tiles();
    },
    
    /** Double-click to zoom and center */
    on_dblclick: function (ev) {
        var offset = this.event_offset(ev);
        
        // center view and zoom in
        this.center_and_zoom_in(
            this.scroll_x + offset.x,
            this.scroll_y + offset.y
        );
    },

    // mousewheel handler
    on_mousewheel: function (ev) {
        // this works in very weird ways, so it's based on code from http://adomas.org/javascript-mouse-wheel/
        // (it didn't include any license, so this is written out manually)
        var delta;
        
        // this is browser-dependant...
        if (ev.wheelDelta) {
            // IE + Opera
            delta = ev.wheelDelta;

            if (window.opera) {  
                // Opera, but apparently not newer versions?
                //delta = -delta;
            }

        } else if (ev.detail) {
            // Mozilla
            delta = -ev.detail;

        } else {
            // mousewheel not supported...
            return;

        }
        
        // don't scroll the page
        if (ev.preventDefault)
            ev.preventDefault();
        
        // delta > 0 : scroll up, zoom in
        // delta < 0 : scroll down, zoom out
        delta = delta < 0 ? -1 : 1;

        // Firefox's DOMMouseEvent's pageX/Y attributes are broken. layerN is for mozilla, offsetN for IE, seems to work

        // absolute location of the cursor
        var x = parseInt(ev.target.style.left) + (ev.layerX ? ev.layerX : ev.offsetX);
        var y = parseInt(ev.target.style.top) + (ev.layerY ? ev.layerY : ev.offsetY);
        
        // zoom \o/
        this.zoom_center_to(x, y, delta);
    },
    
    /** Substrate scroll was started */
    on_scroll_start: function (ev) {

    },
    
    /** Substrate was scrolled, update scroll_{x,y}, and then update tiles after 100ms */
    on_scroll_move: function (ev) {
        this.update_scroll();
        this.update_after_timeout();
    },
    
    /** Substrate scroll was ended, update tiles now */
    on_scroll_end: function (ev) {
        this.update_now();
    },

    /** Calculate the absolute (x, y) coords of the given event inside the viewport */
    event_offset: function (ev) {
        var offset = this.div.cumulativeOffset();

        return {
            x: ev.pointerX() - offset.left, 
            y: ev.pointerY() - offset.top
        };
    },

/*
 * Change view - scroll/zoom
 */                  

    /** Scroll the view to place the given absolute (x, y) co-ordinate at the top left */
    scroll_to: function (x, y) {
        // update it via the style
        this.substrate.style.top = "-" + y + "px";
        this.substrate.style.left = "-" + x + "px";
        
        // update these as well
        this.scroll_x = x;
        this.scroll_y = y;
    },

    /** Scroll the view to place the given absolute (x, y) co-ordinate at the center */
    scroll_center_to: function (x, y) {
        return this.scroll_to(
            x - this.center_offset_x,
            y - this.center_offset_y
        );
    },
 
    /** Zoom à la delta such that the given (zoomed) absolute (x, y) co-ordinates will be at the top left */
    zoom_scaled: function (x, y, delta) {
        if (!this.update_zoom(delta))
            // couldn't zoom, scaled coords are wrong
            return false;

        // scroll to the new position
        this.scroll_to(x, y);
        
        // update view after 100ms - in case we zoom again?
        this.update_after_timeout();
        
        return true;
    },
   
    /** Zoom à la delta such that the given (current) absolute (x, y) co-ordinates will be at the top left */
    zoom_to: function (x, y, delta) {
        return this.zoom_scaled(
            scaleByZoomDelta(x, delta),
            scaleByZoomDelta(y, delta),
            delta
        );
    },
    
    /** Zoom à la delta such that the given (current) absolute (x, y) co-ordinates will be at the center */
    zoom_center_to: function (x, y, delta) {
        return this.zoom_scaled(
            scaleByZoomDelta(x, delta) - this.center_offset_x,
            scaleByZoomDelta(y, delta) - this.center_offset_y,
            delta
        );
    },

    /** Zoom à la delta, keeping the view centered */
    zoom_centered: function (delta) {
        return this.zoom_center_to(
            this.scroll_x + this.center_offset_x,
            this.scroll_y + this.center_offset_y,
            delta
        );
    },

    /** Zoom in one level, keeping the view centered */
    zoom_in: function () {
        return this.zoom_centered(+1);
    },
    
    /** Zoom out one level, keeping the view centered */
    zoom_out: function () {
        return this.zoom_centered(-1);
    },

    /** Center the view on the given coords, and zoom in, if possible */
    center_and_zoom_in: function (cx, cy) {
        // try and zoom in
        if (this.update_zoom(1)) {
            // scaled coords
            cx = scaleByZoomDelta(cx, 1);
            cy = scaleByZoomDelta(cy, 1);
        }

        // re-center
        this.scroll_center_to(cx, cy);
        
        // update view after 100ms - in case we zoom again?
        this.update_after_timeout();
    },

/*
 * Update view state
 */              
 
    /** Update the view_* / center_offset_* vars, and any dependent items */
    update_size: function () {
        this.view_width = this.div.getWidth();
        this.view_height = this.div.getHeight();

        this.center_offset_x = Math.floor(this.view_width / 2);
        this.center_offset_y = Math.floor(this.view_height / 2);
        
        // the link-to-image uses the current view size
        this.update_image_link();
    },
   
    /**
     * Update the scroll_x/y state
     */    
    update_scroll: function() {
        this.scroll_x = -parseInt(this.substrate.style.left);
        this.scroll_y = -parseInt(this.substrate.style.top);
    },
    
    /**
     * Switch zoom layers
     */
    update_zoom: function (delta) {
        // is the new zoom level valid?
        if (!this.zoom_layers[this.zoom_layer.level + delta])
            return false;

        var zoom_old = this.zoom_layer;
        var zoom_new = this.zoom_layers[this.zoom_layer.level + delta];
        
        // XXX: clear hide-zoom-after-timeout
        if (this.zoom_timer) {
            clearTimeout(this.zoom_timer);
            this.zoom_timer = null;
        }
        
        // get other zoom layers out of the way
        // XXX: u
        this.zoom_layers.each(function (zl) {
            zl.disable();
        });
        
        // update the current zoom layer
        this.zoom_layer = zoom_new;
        
        // layer them such that the old on remains visible underneath the new one
        zoom_new.enable(11);
        zoom_old.enable(10);
        
        // resize the tiles in the two zoom layers
        zoom_new.update_tiles(zoom_new.level);
        zoom_old.update_tiles(zoom_new.level);
        
        // disable the old zoom layer after 1000ms - after the new zoom layer has loaded - not an optimal solution
        this.zoom_timer = setTimeout(function () { zoom_old.disable()}, 1000);

        // update UI state
        this.update_zoom_ui();

        return true;
    },

    /** Schedule an update_tiles() after a 100ms interval */
    update_after_timeout: function () {
        // have not called update_tiles() yet
        this._update_idle = false;
        
        // cancel old timeout
        if (this._update_timeout)
            clearTimeout(this._update_timeout);
        
        // trigger in 100ms
        this._update_timeout = setTimeout(this._update_timeout_trigger.bind(this), 100);  
    },
    
    _update_timeout_trigger: function () {
        // have called update_tiles()
        this._update_idle = true;
        
        this.update_tiles();
    },

    /** 
     * Unschedule the call to update_tiles() and call it now, unless it's already been triggered by the previous call to
     * update_after_timeout
     */
    update_now: function () {
        // abort trigger if active
        if (this._update_timeout)
            clearTimeout(this._update_timeout);
        
        // update now unless already done
        if (!this._update_idle)
            this.update_tiles();
    },

    /**
     * Determine the set of visible tiles, and ensure they are loaded
     */
    update_tiles: function () {
        // short names for some vars...
        var x = this.scroll_x;
        var y = this.scroll_y;
        var sw = this.view_width;
        var sh = this.view_height;
        var tw = this.source.tile_width;
        var th = this.source.tile_height;
        var zl = this.zoom_layer.level;
        
        // figure out which set of cols/rows are visible 
        var start_col = Math.max(0, Math.floor(x / tw));
        var start_row = Math.max(0, Math.floor(y / th));
        var end_col = Math.floor((x + sw) / tw);
        var end_row = Math.floor((y + sh) / th);

        // loop through all visible tiles
        for (var col = start_col; col <= end_col; col++) {
            for (var row = start_row; row <= end_row; row++) {
                // the tile's id
                var id = "tile_" + zl + "_" + col + "_" + row;
                
                // does the element exist already?
                // XXX: use basic document.getElementById for perf?
                var t = $(id);

                if (!t) {
                    // build a new tile
                    t = Builder.node("img", {
                            src:    this.source.build_url(col, row, zl /* , sw, sh */),
                            id:     id //,
                            // title:  "(" + col + ", " + row + ")",
                            // style set later
                        }
                    );
                    
                    // position
                    t.style.top = th * row;
                    t.style.left = tw * col;
                    t.style.display = "none";
                    
                    // display once loaded
                    Event.observe(t, "load", _tile_loaded.bindAsEventListener(t));

                    // remember the col/row
                    t.__col = col;
                    t.__row = row;
                    
                    // add it to the zoom layer
                    this.zoom_layer.add_tile(t);

                } else if (this.source.reload) {
                    // force the tile to reload
                    touch_tile(t, col, row);

                }
            }
        }
        
        this.update_scroll_ui();
    }, 

/*
 * UI state
 */                  

    /**
     * Update any zl-dependant UI elements
     */
    update_zoom_ui: function () {
        // deactivate zoom-in button if zoomed in
        if (this.btn_zoom_in)
            (this.zoom_layer.level >= this.source.zoom_max) ? this.btn_zoom_in.disable() : this.btn_zoom_in.enable();
        
        // deactivate zoom-out button if zoomed out
        if (this.btn_zoom_out)
            (this.zoom_layer.level <= this.source.zoom_min) ? this.btn_zoom_out.disable() : this.btn_zoom_out.enable();
        
        // link-to-image
        this.update_image_link();
    },
 
    /**
     * Update any position-dependant UI elements
     */ 
    update_scroll_ui: function () {
        // update the link-to-this-page thing
        document.location.hash = (this.scroll_x + this.center_offset_x) + ":" + (this.scroll_y + this.center_offset_y) + ":" + this.zoom_layer.level;
        
        // update link-to-image
        this.update_image_link();
    },
    
    /**
     * Update the link-to-image-of-this-view link with dimensions, zoom, position
     */
    update_image_link: function () {
        if (!this.image_link)
            return;

        this.image_link.href = this.source.build_image_url(
            this.scroll_x + this.center_offset_x,
            this.scroll_y + this.center_offset_y,
            this.view_width, this.view_height,
            this.zoom_layer.level
        );

        this.image_link.innerHTML = this.view_width + "x" + this.view_height + "@" + this.zoom_layer.level;
    }
});

/** Used by Viewport.update_tiles to make a tile visible after it has loaded */
function _tile_loaded (ev) {
    this.style.display = "block";
}

/**
 * A zoom layer contains a (col, row) grid of tiles at a specific zoom level. 
 */
var ZoomLayer = Class.create({
    initialize: function (source, zoom_level) {
        this.source = source;
        this.level = zoom_level;
        this.div = Builder.node("div", { id: "zl_" + this.level, style: "position: relative; display: none;"});

        // our tiles
        this.tiles = [];
    },
    
    /** Add a tile to this zoom layer's grid */
    add_tile: function (tile) {
        this.div.appendChild(tile);
        this.tiles.push(tile);
    },
    
    /** Make this zoom layer visible with the given z-index */
    enable: function (z_index) {
        this.div.style.zIndex = z_index;

        // XXX: IE8 needs this for some reason
        $(this.div).show();
    },
   
    /** Hide this zoom layer */
    disable: function () {
        // XXX: IE8
        $(this.div).hide();
    },
    
    /** 
     * Update the tile grid in this zoom layer such the tiles are in the correct position and of the correct size
     * when viewed with the given zoom level.
     *
     * For zoom levels different than this layer's level, this will resize the tiles!
     */
    update_tiles: function (zoom_level) {
        var zd = zoom_level - this.level;
        
        // desired tile size
        var tw = scaleByZoomDelta(this.source.tile_width, zd);
        var th = scaleByZoomDelta(this.source.tile_height, zd);

        var tiles = this.tiles;
        var tiles_len = tiles.length;
        var t, ts;
        
        // XXX: *all* tiles? :/
        for (var i = 0; i < tiles_len; i++) {
            t = tiles[i];
            ts = t.style;
            
            // reposition
            ts.width = tw;
            ts.height = th;
            ts.top = th * t.__row;
            ts.left = tw * t.__col;
        }
    }
});

// scale the given co-ordinate by a zoom delta. If we zoom in (dz > 0), n will become larger, and if we zoom 
// out (dz < 0), n will become smaller.
function scaleByZoomDelta (n, dz) {
    if (dz > 0)
        return n << dz;
    else
        return n >> -dz;
}

