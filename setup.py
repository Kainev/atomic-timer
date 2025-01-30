import sys
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import pybind11

class get_pybind_include(object):
    def __str__(self):
        return pybind11.get_include()

class get_pybind_include_user(object):
    def __str__(self):
        return pybind11.get_include(user=True)

libraries = []
if sys.platform.startswith('win'):
    libraries.append('ws2_32')
else:
    libraries.append('pthread')

ext_modules = [
    Extension(
        'atomic_timer',
        [
            'bindings/python.cpp',
            'source/AtomicTimer.cpp',
            'source/NTPClient.cpp',
        ],
        include_dirs=[
            get_pybind_include(),
            get_pybind_include_user(),
            'source' 
        ],
        libraries=libraries,
        language='c++',
        extra_compile_args=['-std=c++20'],
    ),
]


class BuildExt(build_ext):
    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = []
        if ct == 'unix':
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
            opts.append('-std=c++20')
            opts.append('-fvisibility=hidden')
            if sys.platform == 'darwin':
                opts.append('-stdlib=libc++')
                opts.append('-mmacosx-version-min=10.7')
        elif ct == 'msvc':
            opts.append('/DVERSION_INFO=\\"%s\\"' % self.distribution.get_version())
            opts.append('/std:c++20')
        for ext in self.extensions:
            ext.extra_compile_args = opts
        build_ext.build_extensions(self)

setup(
    name='atomic_timer',
    version='0.1.0',
    author='Kainev',
    ext_modules=ext_modules,
    cmdclass={'build_ext': BuildExt},
    zip_safe=False,
    install_requires=['pybind11>=2.6'],
    classifiers=[
        'Programming Language :: Python :: 3',
        'Programming Language :: C++',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.6',
)
