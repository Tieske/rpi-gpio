import distribute_setup
distribute_setup.use_setuptools()
from setuptools import setup, Extension

classifiers = ['Development Status :: 3 - Alpha',
               'Operating System :: POSIX :: Linux',
               'License :: OSI Approved :: MIT License',
               'Intended Audience :: Developers',
               'Programming Language :: Python :: 2.6',
               'Programming Language :: Python :: 2.7',
               'Programming Language :: Python :: 3',
               'Topic :: Software Development',
               'Topic :: Home Automation',
               'Topic :: System :: Hardware']

setup(name             = 'RPi.GPIO',
      version          = '0.4.0a',
      author           = 'Ben Croston',
      author_email     = 'ben@croston.org',
      description      = 'A class to control Raspberry Pi GPIO channels',
      long_description = open('README.txt').read() + open('CHANGELOG.txt').read(),
      license          = 'MIT',
      keywords         = 'Raspberry Pi GPIO',
      url              = 'http://code.google.com/p/raspberry-gpio-python/',
      classifiers      = classifiers,
      ext_modules      = [Extension('RPi.GPIO', ['source/py_gpio.c', 'source/c_gpio.c', 'source/cpuinfo.c'])])
