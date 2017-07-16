# pypngtile

Python extension for pngtile

## Dependencies

* python-cython
* python-dev

## Development

    $ python setup.py build_ext

## Install

    $ virtualenv /opt/pngtile
    $ make -C .. -B install PREFIX=/opt/pngtile
    $ /opt/pngtile/bin/pip install -r requirements.txt
    $ /opt/pngtile/bin/python setup.py build_ext -I /opt/pngtile/include -L /opt/pngtile/lib -R /opt/pngtile/lib
    $ /opt/pngtile/bin/python setup.py install
