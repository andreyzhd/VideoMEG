# -*- coding: utf-8 -*-

import os
from os import path as op

try:
    # use setuptools namespace, allows for "develop"
    import setuptools  # noqa, analysis:ignore
except ImportError:
    pass  # it's not essential for installation
from distutils.core import setup

name = 'pyvideomeg'
description = 'Tools for MEG video data'


# Get version and docstring
__version__ = '0.1.1'
__doc__ = ''
docStatus = 0  # Not started, in progress, done


def package_tree(pkgroot):
    path = os.path.dirname(__file__)
    subdirs = [os.path.relpath(i[0], path).replace(os.path.sep, '.')
               for i in os.walk(os.path.join(path, pkgroot))
               if '__init__.py' in i[2]]
    return subdirs

if __name__ == '__main__':
    setup(
        name=name,
        version=__version__,
        author='VideoMEG contributors',
        author_email='andrey.zhdanov@aalto.fi',
        license='GPL v3',
        url='http://github.com/andreyzhd/VideoMEG',
        keywords="medical imaging",
        description=description,
        long_description=__doc__,
        platforms='any',
        provides=['pyvideomeg'],
        install_requires=['numpy', 'pillow'],
        packages=package_tree('pyvideomeg'),
        package_dir={
            'pyvideomeg': 'pyvideomeg'},
        package_data={},
        zip_safe=False,
        classifiers=[
            'Development Status :: 3 - Alpha',
            'Intended Audience :: Science/Research',
            'Intended Audience :: Education',
            'Intended Audience :: Developers',
            'Operating System :: MacOS :: MacOS X',
            'Operating System :: Microsoft :: Windows',
            'Operating System :: POSIX',
            'Programming Language :: Python',
            'Programming Language :: Python :: 2.6',
            'Programming Language :: Python :: 2.7',
            'Programming Language :: Python :: 3.2',
            'Programming Language :: Python :: 3.3',
        ],
        scripts=[
            op.join('bin', 'pvm_data_converter.py'),
            op.join('bin', 'pvm_data_converter0_1.py'),
            op.join('bin', 'pvm_export.py'),
            op.join('bin', 'pvm_export_audio.py'),
            op.join('bin', 'pvm_export_dragdrop.py'),
            op.join('bin', 'pvm_repack_audio.py'),
            op.join('bin', 'pvm_show_info.py'),
        ],
    )
