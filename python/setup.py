#!/usr/bin/env python
"""pyCANopen: Python bindings to the libCANopen library from rSCADA.

pyCANopen offers binding to the libCANopen C library, which is designed
to be used with the Linux SocketCAN driver.
"""

DOCLINES = __doc__.split('\n')

CLASSIFIERS = """\
Programming Language :: Python
Topic :: Engineering
Operating System :: POSIX
Operating System :: Unix
"""

from distutils.core import setup

MAJOR               = 0
MINOR               = 0
MICRO               = 1

setup(
    name = "pyCANopen",
    version = "%d.%d.%d" % (MAJOR, MINOR, MICRO),
    packages = ['pycanopen'],
    scripts = ['examples/canopen-node-info.py', 'examples/canopen-dump.py'],
    author = "Robert Johansson",
    author_email = "rob@raditex.nu",
    license = "BSD",
    description = DOCLINES[0],
    long_description = "\n".join(DOCLINES[2:]),
    keywords = "CANopen library for python",
    url = "http://www.rscada.se",
    platforms = ["Linux", "Unix"],
    )
