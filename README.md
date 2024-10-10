
(under construction ...)

official github: https://github.com/openbmc/sdbusplus

sudo apt install git meson libtool pkg-config g++ libsystemd-dev \
    python3 python3-pip python3-yaml python3-mako python3-inflection


The sdbusplus library is built using meson

meson build
cd build
ninja
ninja test
ninja install
Optionally, building the tests and examples can be disabled by passing -Dtests=disabled and -Dexamples=disabled respectively to meson.

The sdbus++ application is installed as a standard Python package using setuptools

cd tools
./setup.py install
