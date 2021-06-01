"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
import os
import platform

from setuptools import setup, find_packages
from setuptools.command.develop import develop
from setuptools.command.build_py import build_py

PACKAGE_ROOT = os.path.abspath(os.path.dirname(__file__))

PYTHON_64 = platform.architecture()[0] == '64bit'


if __name__ == '__main__':
    if not PYTHON_64:
        raise RuntimeError("32-bit Python is not a supported platform.")

    with open(os.path.join(PACKAGE_ROOT, 'README.txt')) as f:
        long_description = f.read()

    setup(
        name="o3de",
        version="1.0.0",
        description='O3DE editor Python bindings test tools',
        long_description=long_description,
        packages=find_packages(where='o3de', exclude=['tests']),
        install_requires=[
        ],
        tests_require=[
        ],
        entry_points={
        },
    )
