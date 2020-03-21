from setuptools import setup, find_packages, Extension
import sys
from wheel.bdist_wheel import bdist_wheel as _bdist_wheel

import os
import glob

class bdist_wheel(_bdist_wheel):

    def finalize_options(self):
        _bdist_wheel.finalize_options(self)
        # Mark us as not a pure python package
        self.root_is_pure = False

    def get_tag(self):
        python, abi, plat = _bdist_wheel.get_tag(self)
        # We don't contain any python source
        python, abi = 'py3', 'none'
        return python, abi, plat

install_path = os.path.dirname(os.path.realpath(__file__))
lib_path = install_path + '/../../build/'

from sys import platform
if "linux" in platform:
    # linux
    lib = lib_path + 'libaps2.so'
elif platform == "darwin":
    # OS X
    lib = lib_path + 'libaps2.dylib'
elif platform == "win32":
    # Windows...
    lib = lib_path + 'libaps2.dll'

setup(
    name='libaps2',
    version='2019.1',
    author='BBN-Q Developers',
    cmdclass={'bdist_wheel': bdist_wheel},
    url='https://github.com/BBN-Q/libaps2',
    download_url='https://github.com/BBN-Q/auspex',
    license="Apache 2.0 License",
    description='BBN APS2 instrument control library.',
    long_description=open(install_path + '/../../README.md').read(),
    long_description_content_type='text/markdown',
    py_modules=["aps2"],
    data_files=[('lib', [lib]),
    ('utils', glob.glob(os.path.join('.', lib_path, 'aps2_*')))],
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: MacOS",
        "Operating System :: POSIX",
        "Operating System :: Microsoft :: Windows :: Windows 10",
        "Programming Language :: Python :: 3 :: Only",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Topic :: Scientific/Engineering",
    ],
    python_requires='>=3.6',
    install_requires=[
        'numpy>=1.18'
    ],
    keywords="quantum qubit arbitrary waveform instrument hardware"
)
