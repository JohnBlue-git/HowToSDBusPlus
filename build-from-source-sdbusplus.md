
# About this mark down
To note the buuild process of sdbusplus library, and to explain the meson build script regard building sdbusplus library.

## Official website
https://github.com/openbmc/sdbusplus

## Pre-install some dependency
```console
sudo apt-get install libboost-all-dev
# should >= 1.81
dpkg -l | grep libboost
# or
cat /usr/include/boost/version.h

sudo apt install git meson libtool pkg-config

# systemd: libsystemd-dev
# libsystemd-dev: libcap-dev(sys/capability.h), gperf
sudo apt install g++ gperf libcap-dev libsystemd-dev

# python
sudo apt install python3 python3-pip python3-yaml python3-mako python3-inflection
# python3-xyz or xyz
pip3 install mako inflection
```

## Pre-install Boost
```console
# (way 1)

# check available version
apt-cache policy libboost-all-dev

# apt install
sudo apt install libboost-all-dev=1.81

# to remove
sudo apt remove libboost-all-dev
sudo apt autoremove

# (way 2)

# download source
wget https://archives.boost.io/release/1.81.0/source/boost_1_81_0.tar.gz

# extract
tar -xzf boost_1_81_0.tar.gz

# navigate to folder
cd boost_1_81_0

# boostrap and build (install b2 to /usr/bin)
sudo ./bootstrap.sh --prefix=/usr

# install (to /usr/lib and /usr/include/boost)
sudo ./b2 install --prefix=/usr
# install to local
# prefix=/usr/local

# to remove
# sudo rm -rf /usr/include/boost
# sudo rm -rf /usr/lib/libboost*
# library installed by apt may lie in
# sudo rm -rf /usr/lib/x86_64-linux-gnu/libboost*
```
Another way is to build as package and install via dpkg (then can be managed by dpkg and apt)
```console
# Bootstrap and Build Boost
sudo ./bootstrap.sh --prefix=/usr
sudo ./b2 --prefix=/usr

# Create Debian Package: Now, use dh_make to create a Debian package. 
# Choose "Single binary" by entering "s":
# Please move boost_1_81_0 to boost_1.81.0 for dh_make to find the package name and version, the current directory
needs to be in the format of <package>-<version>
dh_make -p boost_1.81.0 --createorig -s -y

# Customize and Build the Package
# Edit the debian/control file to update the package details, and then build the package ...
# (Once built, the .deb file will be in the parent directory)
dpkg-buildpackage -us -uc

# Install the Package
sudo dpkg -i

# The information will be stored in
# /var/lib/dpkg/available: Lists available packages1
# /var/lib/dpkg/status:    Contains the statuses of available packages1
# /var/lib/dpkg/triggers:  Manages package triggers
```

## Pre-install Meson from Source (if the package manager doesn't have the latest version):
Normally, just do the following ...
```console
git clone https://github.com/mesonbuild/meson.git
cd meson
# this will install in /home/codespace/.local/current/bin/meson
python3 setup.py install --user
# this will install in /home/codespace/.python/current/bin/meson
python3 setup.py install
```
To prevent conflicts between system package managers and Python’s package manager. Here are a few ways to resolve this issue: \
(way 1) Use a Virtual Environment: Create a virtual environment to isolate your Python packages from the system packages
```console
sudo apt install python3-venv
python3 -m venv ~/myenv
source ~/myenv/bin/activate
pip3 install --upgrade meson
```
(way 2) Use pipx: pipx is a tool to install and run Python applications in isolated environments
```console
sudo apt install pipx
pipx ensurepath
pipx install meson
```
Note that:
- Using a virtual environment or pipx is generally the safest approach as it avoids potential conflicts with system packages
- The meson will be installed on /home/johnblue/.local/bin/meson instead of /usr/bin/meson
```console
# we can use soft-link to pretend to be a /usr/bin/meson
cd /usr/bin/ && sudo ln -s /home/johnblue/.local/bin/meson meson
```

## Pre-install libsystemd-dev 
libsystemd-dev shall upgrade to a newer version (you need version 253 or higher)
```
# check version
dpkg -l | grep libsystemd-dev

# pre-install
pip3 install --user jinja2

# install from source code
git clone https://github.com/systemd/systemd.git
cd systemd
git checkout v253
env PATH=/usr/bin:$PATH meson setup build
cd build
ninja
sudo ninja install
```

## Pre-install python package for tools
Install jsonschema
```console
# update pip
pip install --upgrade pip

# Install jsonschema
# pip install jsonschema
# To ensure you're using pip for Python 3, you can explicitly call:
pip3 install jsonschema

# Verify
python3 -m jsonschema --version
```

## To force the installation of GCC G++ 13:
```console
# Add the Toolchain PPA (if necessary): If GCC 13 is not available in your default repositories, you can add the Ubuntu Toolchain PPA
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update

# Install GCC 13
sudo apt install gcc-13 g++-13

# Set GCC 13 as the Default
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 60
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 60

# configure which version to use by running:
sudo update-alternatives --config gcc
sudo update-alternatives --config g++

# Verify
gcc --version
```

## Clone the lbrary source
```console
git clone https://github.com/openbmc/sdbusplus.git
cd sdbusplus
```

## Built via meson
Optionally, building the tests and examples can be disabled by passing -Dtests=disabled and -Dexamples=disabled respectively to meson.
```console
sudo meson setup --reconfigure build
cd build
sudo ninja
ninja test
sudo ninja install
# because we have to use sudo finally, sudo all the command is more convient
```
Some sdbus++ applications will need to be installed as a standard Python package using setuptools
```console
cd tools
./setup.py install
```

## If want to update subproject
```console
meson subprojects update
```

## If encounter boost library header deprecated
reference: \
https://stackoverflow.com/questions/66906613/how-to-silence-internal-boost-deprecated-messages

Add to meson.build
```console
boost_compile_args = [
    '-DBOOST_ALLOW_DEPRECATED_HEADERS',

]
```

## Explain the meson build script

### meson.options
It is for customization about which to build (we can edit this to manipulate)
```meson
option('tests', type: 'feature', description: 'Build tests')
option('examples', type: 'feature', description: 'Build examples')
```

### meson.build
It is the building script

### Setting project
```meson
project('sdbusplus', 'cpp', 'c',
    default_options: [
      'buildtype=debugoptimized',
      'cpp_std=c++23',
      'warning_level=3',
      'werror=true',
      'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
      'examples=' + (meson.is_subproject() ? 'disabled' : 'auto'),
    ],
    version: '1.0.0',
    meson_version: '>=1.1.1',
)
```
notice:
- when meson.build in sub-directory contains "subproject(< ... >))", meson.is_subproject() will return "true"

### find libraries

- dependency()
	- Finding the pkg-config dependency (would possible to use pkgconfig to collect neccessary meta data)

- include_type: 'system'
	- Informs meson that the headers for this dependency should be treated as system headers
	- Warning Suppression (useful when using third-party libraries)
	- Cross-Platform Compatibility

### subprojects directory and .wrap
subprojects directory would contains .wrap files

- Dependency Management: 
	- .wrap file specifies an external library or project that your main project depends on
	- It allows meson to fetch and integrate the specified library automatically
	- Configuration:
		- The source repository (e.g., a Git URL).
		- The specific version or branch to check out.
		- Any additional instructions or options for the fetch process.

- Isolation:
	- Subprojects allow dependencies to be isolated from the main project, helping avoid conflicts between different versions of the same library that might be needed by various projects

- Automatic Downloading:
	- When you run **meson setup**, Meson can automatically download the specified dependency if it’s not already present in the subprojects directory

subprojects/nlohmann_json.wrap
```meson
[wrap-git]
revision = HEAD
url = https://github.com/nlohmann/json.git

[provide]
nlohmann_json = nlohmann_json_dep
```
meson.buid
```meson
libsystemd_pkg = dependency('libsystemd')
nlohmann_json_dep = dependency('nlohmann_json', include_type: 'system')
```

### Forcelly require python3
Because some tool must use python3
```meson
python = import('python')
python_bin = python.find_installation('python3', modules:['inflection', 'yaml', 'mako'])

if not python_bin.found()
  error('No valid python3 installation found')
endif
```

### Iclude header / source files
Header files in ./include \
Source code in ./src
```meson
root_inc = include_directories('include')

libsdbusplus_src = files(
    'src/async/context.cpp',
    'src/async/match.cpp',
    'src/bus.cpp',
    'src/bus/match.cpp',
    'src/event.cpp',
    'src/exception.cpp',
    'src/message/native_types.cpp',
    'src/sdbus.cpp',
    'src/server/interface.cpp',
    'src/server/transaction.cpp',
)
```

### Add library
```meson
libsdbusplus = library(
    'sdbusplus',
    libsdbusplus_src,
    include_directories: root_inc,
    dependencies: [
        libsystemd_pkg,
        nlohmann_json_dep
    ],
    version: meson.project_version(),
    install: true,
)
```

### Install header files
- "include/sdbusplus" directory should be installed
- "install_dir" specifies the destination directory where the contents will be installed
- "get_option('includedir')" represents the standard include directory for header files (like /usr/include or a similar path)
- "strip_directory: false": This option determines whether the directory structure should be stripped during installation. Setting strip_directory to false means that the directory structure (include/sdbusplus) will be preserved in the installation path
```meson
install_subdir(
    'include/sdbusplus',
    install_dir: get_option('includedir'),
    strip_directory: false,
)
```

### Create a .pc (pkg-config) file
This file provides metadata about the library
- Provide essential metadata (name, version, dependencies) that helps manage library dependencies
- Ensure that any necessary compiler flags (like the Boost-related flags) are automatically included when compiling code that uses the library
- By default, the generated .pc file will be installed in the standard pkg-config directory, which is typically something like /usr/lib/pkgconfig or /usr/share/pkgconfig or similar
- To custom installation path "install_dir: join_paths(get_option('libdir'), 'pkgconfig')"
```meson
import('pkgconfig').generate(
    libsdbusplus,
    name: meson.project_name(),
    version: meson.project_version(),
    requires: libsystemd_pkg,
    extra_cflags: boost_compile_args,
    description: 'C++ bindings for sdbus',
)
```

### Boost library (for sub-directory / application)
Setting boost compile arguments

- DBOOST_ASIO_DISABLE_THREADS
	- Disables threading in Boost.Asio, making it single-threaded.

- DBOOST_ALL_NO_LIB
	- Prevents automatic linking of Boost libraries, requiring manual linking instead.

- DBOOST_SYSTEM_NO_DEPRECATED
	- Suppresses deprecated features in the Boost.System library.

- DBOOST_ERROR_CODE_HEADER_ONLY
	- Makes Boost error codes header-only.

- DBOOST_COROUTINES_NO_DEPRECATION_WARNING
	- Suppresses deprecation warnings for Boost Coroutines.

```meson
boost_compile_args = [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
]

boost_dep = declare_dependency(
    dependencies: dependency('boost', required: false),
    compile_args: boost_compile_args)
```
.wrap
```meson
[wrap-git]
directory = boost
url = https://github.com/boostorg/boost.git
revision = master  # or specify a version/tag
```

### Define dependency becauuse application want to use sdbusplus library
```meson
sdbusplus_dep = declare_dependency(
    include_directories: root_inc,
    link_with: libsdbusplus,
    dependencies: [
        boost_dep,
        libsystemd_pkg,
        nlohmann_json_dep,
    ],
)
```

### Build tools for sdbusplus project (can create header and source from .yaml ...)
```meson
subdir('tools')
```

### Build the sub-directories ( "examples" and "tests" )
```meson
if not get_option('examples').disabled()
  subdir('example')
endif
if not get_option('tests').disabled()
  subdir('test')
endif
```
