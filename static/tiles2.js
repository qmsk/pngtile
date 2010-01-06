// A source of tile images of a specific width/height, zoom level range, and some other attributes
var Source = Class.create({
    initialize: function (path, tile_width, tile_height, zoom_min, zoom_max) {
        this.path = path;
        this.tile_width = tile_width;
        this.tile_height = tile_height;
        this.zoom_min = zoom_min;
        this.zoom_max = zoom_max;

        this.refresh = false;
        this.opt_key = this.opt_value = null;
    },
    
    // build a URL for the given tile image
    build_url: function (col, row, zl, sw, sh) {
        // two-bit hash (0-4) based on the (col, row)
        var hash = ( (col % 2) << 1 | (row % 2) ) + 1;
        
        // the subdomain to use
        var subdomain = "";
        
        if (0)
            subdomain = "tile" + hash + ".";

        // the (x, y) co-ordinates
        var x = col * this.tile_width;
        var y = row * this.tile_height;

        var url = this.path + "?x=" + x + "&y=" + y + "&z=" + zl + "&sw=" + sw + "&sh=" + sh;

        if (this.refresh)
            url += "&ts=" + new Date().getTime();

        if (this.opt_key && this.opt_value)
            url += "&" + this.opt_key + "=" + this.opt_value;

        return url;
    },
});

// a viewport that contains a substrate which contains several zoom layers which contain many tiles
var Viewport = Class.create({
    initialize: function (source, viewport_id) {
        this.source = source;

        this.id = viewport_id;
        this.div = $(viewport_id);
        this.substrate = this.div.down("div.substrate");
    
        // the stack of zoom levels
        this.zoom_layers = [];
        
        // pre-populate the stack
        for (var zoom_level = source.zoom_min; zoom_level <= source.zoom_max; zoom_level++) {
            var zoom_layer = new ZoomLayer(source, zoom_level);

            this.substrate.appendChild(zoom_layer.div);
            this.zoom_layers[zoom_level] = zoom_layer;
        }

        // make the substrate draggable
        this.draggable = new Draggable(this.substrate, {
            onStart: this.on_scroll_start.bind(this),
            onDrag: this.on_scroll_move.bind(this),
            onEnd: this.on_scroll_end.bind(this),
        });

        // event handlers
        Event.observe(this.substrate, "dblclick", this.on_dblclick.bindAsEventListener(this));
        Event.observe(this.substrate, "mousewheel", this.on_mousewheel.bindAsEventListener(this));
        Event.observe(this.substrate, "DOMMouseScroll", this.on_mousewheel.bindAsEventListener(this));     // mozilla
        Event.observe(document, "resize", this.on_resize.bindAsEventListener(this));
    
        // set viewport size
        this.update_size();
        
        // initial location?    
        if (document.location.hash) {
            // x:y:z tuple
            var pt = document.location.hash.substr(1).split(":");
            
            // unpack
            var cx = 0, cy = 0, z = 0;
            
            if (pt.length) cx = parseInt(pt.shift());
            if (pt.length) cy = parseInt(pt.shift());
            if (pt.length) z = parseInt(pt.shift());

            // initial view
            this.zoom_center_to(cx, cy, z);

        } else {
            // this sets the scroll offsets, zoom level, and loads the tiles
            this.zoom_to(0, 0, 0);
        }
    },
    
    /* event handlers */

    // window resized
    on_resize: function (ev) {
        this.update_size();
        this.update_tiles();
    },
    
    // double-click handler
    on_dblclick: function (ev) {
        var offset = this.event_offset(ev);
        
        this.zoom_center_to(
            this.scroll_x + offset.x,
            this.scroll_y + offset.y,
            1   // zoom in
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
    
    // substrate scroll was started
    on_scroll_start: function (ev) {

    },
    
    // substrate was scrolled, update scroll_{x,y}, and then update tiles after 100ms
    on_scroll_move: function (ev) {
        this.update_scroll();
        this.update_after_timeout();
    },
    
    // substrate scroll was ended, update tiles now
    on_scroll_end: function (ev) {
        this.update_now();
    },

    /* get state */

    // return the absolute (x, y) coords of the given event inside the viewport
    event_offset: function (ev) {
        var offset = this.div.cumulativeOffset();

        return {
            x: ev.pointerX() - offset.left, 
            y: ev.pointerY() - offset.top
        };
    },

    /* modify state */

    // scroll the view to place the given absolute (x, y) co-ordinate at the top left
    scroll_to: function (x, y) {
        // update it via the style
        this.substrate.style.top = "-" + y + "px";
        this.substrate.style.left = "-" + x + "px";
        
        // update these as well
        this.scroll_x = x;
        this.scroll_y = y;
    },

    // scroll the view to place the given absolute (x, y) co-ordinate at the center
    scroll_center_to: function (x, y) {
        return this.scroll_to(
            x - this.center_offset_x,
            y - this.center_offset_y
        );
    },
 
    // zoom à la delta such that the given (zoomed) absolute (x, y) co-ordinates will be at the top left
    zoom_scaled: function (x, y, delta) {
        if (!this.update_zoom(delta))
            return false;

        // scroll to the new position
        this.scroll_to(x, y);
        
        // update view
        // XXX: ...
        this.update_after_timeout();
        
        return true;
    },
   
    // zoom à la delta such that the given (current) absolute (x, y) co-ordinates will be at the top left
    zoom_to: function (x, y, delta) {
        return this.zoom_scaled(
            scaleByZoomDelta(x, delta),
            scaleByZoomDelta(y, delta),
            delta
        );
    },
    
    // zoom à la delta such that the given (current) absolute (x, y) co-ordinates will be at the center
    zoom_center_to: function (x, y, delta) {
        return this.zoom_scaled(
            scaleByZoomDelta(x, delta) - this.center_offset_x,
            scaleByZoomDelta(y, delta) - this.center_offset_y,
            delta
        );
    },


    /* update view/state to reflect reality */

    // figure out the viewport dimensions
    update_size: function () {
        this.view_width = this.div.getWidth();
        this.view_height = this.div.getHeight();

        this.center_offset_x = Math.floor(this.view_width / 2);
        this.center_offset_y = Math.floor(this.view_height / 2);
    },
    
    // figure out the scroll offset as absolute pixel co-ordinates at the top left
    update_scroll: function() {
        this.scroll_x = -parseInt(this.substrate.style.left);
        this.scroll_y = -parseInt(this.substrate.style.top);
    },

    // wiggle the ZoomLevels around to match the current zoom level
    update_zoom: function(delta) {
        if (!this.zoom_layer) {
            // first zoom operation

            // is the new zoom level valid?
            if (!this.zoom_layers[delta])
                return false;
            
            // set the zoom layyer
            this.zoom_layer = this.zoom_layers[delta];
            
            // enable it
            this.zoom_layer.enable(11);
            
            // no need to .update_tiles or anything like that
            
        } else {
            // is the new zoom level valid?
            if (!this.zoom_layers[this.zoom_layer.level + delta])
                return false;

            var zoom_old = this.zoom_layer;
            var zoom_new = this.zoom_layers[this.zoom_layer.level + delta];
            
            // XXX: ugly hack
            if (this.zoom_timer) {
                clearTimeout(this.zoom_timer);
                this.zoom_timer = null;
            }
            
            // get other zoom layers out of the way
            this.zoom_layers.each(function (zl) {
                zl.disable();
            });
            
            // update the zoom layer
            this.zoom_layer = zoom_new;
            
            // apply new z-indexes, preferr the current one over the new one
            zoom_new.enable(11);
            zoom_old.enable(10);
            
            // resize the tiles in the two zoom layers
            zoom_new.update_tiles(zoom_new.level);
            zoom_old.update_tiles(zoom_old.level);
            
            // XXX: ugly hack
            this.zoom_timer = setTimeout(function () { zoom_old.disable()}, 1000);
        }

        return true;
    },
    
    // ensure that all tiles that are currently visible are loaded
    update_tiles: function() {
        // short names for some vars...
        var x = this.scroll_x;
        var y = this.scroll_y;
        var sw = this.view_width;
        var sh = this.view_height;
        var tw = this.source.tile_width;
        var th = this.source.tile_height;
        var zl = this.zoom_layer.level;
        
        // figure out what set of columns are visible
        var start_col = Math.max(0, Math.floor(x / tw));
        var start_row = Math.max(0, Math.floor(y / th));
        var end_col = Math.floor((x + sw) / tw);
        var end_row = Math.floor((y + sh) / th);

        // loop through all those tiles
        for (var col = start_col; col <= end_col; col++) {
            for (var row = start_row; row <= end_row; row++) {
                // the tile's id
                var id = "tile_" + this.zoom_layer.level + "_" + col + "_" + row;
                
                // does the element exist already?
                var t = $(id);

                if (!t) {
                    // build a new tile
                    t = Builder.node("img", {
                            src:    this.source.build_url(col, row, zl, sw, sh),
                            id:     id,
                            title:  "(" + col + ", " + row + ")",
                            // style set later
                        }
                    );
                    
                    // set the CSS style stuff
                    t.style.top = th * row;
                    t.style.left = tw * col;
                    t.style.display = "none";
                    
                    // wait for it to load
                    Event.observe(t, "load", _tile_loaded.bindAsEventListener(t));

                    // store the col/row
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

        // update the link-to-this-page thing
        document.location.hash = "#" + (this.scroll_x + this.center_offset_x) + ":" + (this.scroll_y + this.center_offset_y) + ":" + this.zoom_layer.level;
    }, 

    // do update_tiles after 100ms, unless we are called again
    update_after_timeout: function () {
        this._update_idle = false;

        if (this._update_timeout)
            clearTimeout(this._update_timeout);

        this._update_timeout = setTimeout(this._update_timeout_trigger.bind(this), 100);  
    },
    
    _update_timeout_trigger: function () {
        this._update_idle = true;

        this.update_tiles();
    },

    // call update_tiles if it hasn't been called due to update_after_timeout
    update_now: function () {
        if (this._update_timeout)
            clearTimeout(this._update_timeout);
        
        if (!this._update_idle)
            this.update_tiles();
    },

});

// used by Viewport.update_tiles to make a tile visible after it has loaded
function _tile_loaded (ev) {
    this.style.display = "block";
}

// a zoom layer containing the tiles for one zoom level
var ZoomLayer = Class.create({
    initialize: function (source, zoom_level) {
        this.source = source;
        this.level = zoom_level;
        this.div = Builder.node("div", { id: "zl_" + this.level, style: "position: relative; display: none;"});

        // our tiles
        this.tiles = [];
    },
    
    // add a tile to this zoom layer
    add_tile: function (tile) {
        this.div.appendChild(tile);
        this.tiles.push(tile);
    },
    
    // make this zoom layer visible with the given z-index
    enable: function (z_index) {
        this.div.style.zIndex = z_index;
        this.div.show();
    },
   
    // hide this zoom layer
    disable: function (z_index) {
        this.div.hide();
    },
    
    // update the tiles in this zoom layer so that they are in the correct position and of the correct size when
    // viewed with the given zoom level
    update_tiles: function (zoom_level) {
        var zd = zoom_level - this.level;

        var tw = scaleByZoomDelta(this.source.tile_width, zd);
        var th = scaleByZoomDelta(this.source.tile_height, zd);

        var tiles = this.tiles;
        var tiles_len = tiles.length;
        var t, ts;
        
        for (var i = 0; i < tiles_len; i++) {
            t = tiles[i];
            ts = t.style;

            ts.width = tw;
            ts.height = th;
            ts.top = th*t.__row;
            ts.left = tw*t.__col;
        }
    },



});

// scale the given co-ordinate by a zoom delta. If we zoom in (dz > 0), n will become larger, and if we zoom 
// out (dz < 0), n will become smaller.
function scaleByZoomDelta (n, dz) {
    if (dz > 0)
        return n << dz;
    else
        return n >> -dz;
}

