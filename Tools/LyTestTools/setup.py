"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
import platform

from setuptools import setup, find_packages
from setuptools.command.develop import develop
from setuptools.command.build_py import build_py

PROJECT_ROOT = os.path.abspath(os.path.dirname(__file__))

PYTHON_64 = platform.architecture()[0] == '64bit'


if __name__ == '__main__':
    if not PYTHON_64:
        raise RuntimeError("32-bit Python is not a supported platform.")

    with open(os.path.join(PROJECT_ROOT, 'README.txt')) as f:
        long_description = f.read()

    setup(
        name="ly_test_tools",
        version="1.0.0",
        description='Lumberyard Python Test Tools',
        long_description=long_description,
        packages=find_packages(where='Tools', exclude=['tests']),
        install_requires=[
            'imageio',
            'numpy',
            'pluggy',
            'psutil',
            'pyscreenshot',
            'pytest>=7.2.0',
            'pytest-mock',
            'pytest-timeout',
            'six',
            'scipy',
        ],
        tests_require=[
        ],
        entry_points={
            'pytest11': [
                'ly_test_tools=ly_test_tools._internal.pytest_plugin.test_tools_fixtures',
                'testrail_filter=ly_test_tools._internal.pytest_plugin.case_id',
                'terminal_report=ly_test_tools._internal.pytest_plugin.terminal_report',
                'multi_testing=ly_test_tools._internal.pytest_plugin.multi_testing',
                'pytester=_pytest.pytester'
            ],
        },
    )
