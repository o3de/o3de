"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def Editor_PySide_Imports_Work():
    import importlib

    from editor_python_test_tools.utils import Report

    for module_name in (
        "shiboken6.Shiboken",
        "PySide6.QtConcurrent",
        "PySide6.QtCore",
        "PySide6.QtGui",
        "PySide6.QtNetwork",
        "PySide6.QtOpenGL",
        "PySide6.QtOpenGLWidgets",
        "PySide6.QtSvg",
        "PySide6.QtSvgWidgets",
        "PySide6.QtTest",
        "PySide6.QtUiTools",
        "PySide6.QtWidgets",
        "PySide6.QtXml",
        "az_qt_helpers",
        "show_object_tree",
        "pyside_utils",
    ):
        module = importlib.import_module(module_name)
        Report.result((f"Imported {module_name}", f"Failed to import {module_name}"), module is not None)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Editor_PySide_Imports_Work)
