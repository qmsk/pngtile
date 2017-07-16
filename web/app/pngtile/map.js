var map_config;
var map;

L.Control.Link = L.Control.extend({
    options: {
        position: 'topright',
        url: null,
    },

    onAdd: function (map) {
        var container = L.DomUtil.create('div', 'leaflet-control-link');

        var link = this.link = L.DomUtil.create('a', 'leaflet-control-link-link', container);

        map.on('move', this._update, this);

        return container;
    },

    onRemove: function (map) {
        map.off('move', this._update, this);
    },

    _update: function (e) {
        var map_center = map.getCenter();
        var map_zoom = map.getZoom();
        var size = map.getSize();

        var x = (+map_center.lng) * Math.pow(2, map_config.tile_zoom);
        var y = (-map_center.lat) * Math.pow(2, map_config.tile_zoom);
        var zoom = map_config.tile_zoom - map_zoom;

        var state = {
            w: size.x,
            h: size.y,
            x: x >> zoom,
            y: y >> zoom,
            z: zoom
        };

        var url = L.Util.template(this.options.url, L.extend(state, this.options));

        this.link.href = url;

        with (state) {
            this.link.innerHTML = w + 'x' + h + ' @ (' + x + ', ' + y + ') @ ' + z;
        }
    },
});

L.control.link = function (options) {
    return new L.Control.Link(options);
};

function map_init (_config) {
    map_config = _config;

    // pixel coordinates
    var bounds = L.latLngBounds(
        L.latLng(-map_config.image_height, 0),
        L.latLng(0, +map_config.image_width)
    );

    // in zoom-scaled coordinates
    var map_bounds = [
        [ -(map_config.image_height >> map_config.tile_zoom), 0 ],
        [ 0, +(map_config.image_width >> map_config.tile_zoom) ],
    ];

    map = L.map('map', {
        crs: L              .CRS.Simple,
        minZoom:            0,
        maxZoom:            map_config.tile_zoom,
        maxBounds:          map_bounds
    });

    map.on('move', map_move);

    L.tileLayer(map_config.tile_url, {
        tiles_url:          map_config.tiles_url,
        tiles_mtime:        map_config.tiles_mtime,
        minZoom:            0,
        maxZoom:            map_config.tile_zoom,
        tileSize:           map_config.tile_size,
        continuousWorld:    true,
        noWrap:             true,
        zoomReverse:        true,
        bounds:             bounds
    }).addTo(map);

    // controls
    L.control.link({
        url:        map_config.image_url,
        tiles_url:  map_config.tiles_url,
        tiles_mtime:        map_config.tiles_mtime,
    }).addTo(map);

    // set position
    var x = bounds.getCenter().lng;
    var y = -bounds.getCenter().lat;
    var z = 0;

    if (document.location.hash) {
        // parse x:y:z tuple
        var pt = document.location.hash.substr(1).split(":");

        // unpack
        if (pt.length) x = parseInt(pt.shift()) || x;
        if (pt.length) y = parseInt(pt.shift()) || y;
        if (pt.length) z = parseInt(pt.shift()) || z;
    }

    map_center(x, y, z);
}

function map_move () {
    var map_center = map.getCenter();
    var map_zoom = map.getZoom();

    var x = (+map_center.lng) << map_config.tile_zoom;
    var y = (-map_center.lat) << map_config.tile_zoom;
    var z = map_config.tile_zoom - map_zoom;

    document.location.hash = x + ":" + y + ":" + z;
}

/*
 * Position map based on pngtile coordinates
 */
function map_center (x, y, z) {
    // translate to lat/lng
    // leaflet seems to base its latlng coordinates on the max zoom level
    var map_center = [
        -(y >> map_config.tile_zoom),
        +(x >> map_config.tile_zoom),
    ];

    // reversed zoom
    var map_zoom = map_config.tile_zoom - z;

    map.setView(map_center, map_zoom);
}
