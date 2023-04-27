#!/usr/bin/env python

from setuptools import setup

setup(name='up_purple',
  version='0.1.3',
  description='PURPLE Integration with the UP',
  author='Nicola Gigante',
  author_email='nicola.gigante@unibz.it',
  url='https://www.black-sat.org',
  license = 'MIT',
  license_files = ('LICENSE.txt',),
  py_modules=['up_purple'],
  install_requires=[
    'unified-planning',
    'black-sat',
    'purple-plan',
  ],
  classifiers=[
      'License :: OSI Approved :: MIT License',
      'Topic :: Scientific/Engineering :: Artificial Intelligence'
  ]
)
