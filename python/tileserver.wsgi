import os
import pngtile.tile

application = pngtile.tile.TileApplication(
    image_root      = os.environ['QMSK_PNGTILE_PATH'],
    image_server    = os.environ['QMSK_PNGTILE_IMAGES_URL'],
)

