from distutils.core import setup, Extension

module1 = Extension('MMDB',
	libraries = ['maxminddb'],
	sources = ['py_libmaxmind.c'],
	library_dirs = ['/usr/local/lib'],
	include_dirs = ['/usr/local/include'])

setup (name = 'MMDB-Python',
	version = '1.0.0',
	description = 'This is a python wrapper to libmaxminddb',
	ext_modules = [module1])
