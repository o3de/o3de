# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

import os
import sys
from PySide2 import QtCore
import logging as _logging
import maya_materials
import maya.standalone
maya.standalone.initialize(name='python')
import maya.cmds as mc
import time
import json


_LOGGER = _logging.getLogger('azpy.dcc.maya.utils.standalone_utilities')


class StandaloneUtilities(QtCore.QObject):
    def __init__(self, operation_values, target_operation, parent=None):
        super(StandaloneUtilities, self).__init__(parent)

        self.target_files = []
        self.target_operation = target_operation
        self.transfer_data = {}
        self.set_operation_values(operation_values)
        self.run_operation()

    def set_operation_values(self, operation_values):
        """
        This passed argument can hold one or more variables, which can be used according to the
        target operation setting. The first value will always contain a target file.
        """
        if self.target_operation == 'scene_audit':
            self.target_files = operation_values

    def run_operation(self):
        for file in self.target_files:
            if self.target_operation == 'scene_audit':
                mc.file(file, o=True, force=True)
                scene_info = maya_materials.get_scene_material_information()
                self.transfer_data[file] = scene_info
            elif self.target_operation == 'export':
                pass
            self.return_operation_info()

    def return_operation_info(self):
        try:
            json.dump(self.transfer_data, sys.stdout)
            self.flush_then_wait()
        except Exception as e:
            _LOGGER.info('Standalone Error: {} -- {}'.format(e, self.transfer_data))

    def flush_then_wait(self):
        sys.stdout.flush()
        sys.stderr.flush()
        time.sleep(0.5)


operation_values = sys.argv[1:-1]
target_operation = sys.argv[-1]
instance = StandaloneUtilities(operation_values, target_operation)

