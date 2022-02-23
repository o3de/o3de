"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Pytest test configuration file.

"""
import pathlib

def pytest_addoption(parser):
    parser.addoption("--installer-path", action="store", default=pathlib.Path().resolve())
    parser.addoption("--install-root", action="store", default=pathlib.PurePath('C:/O3DE/0.0.0.0'))
    parser.addoption("--project-path", action="store", default=pathlib.PurePath('C:/Workspace/TestProject'))