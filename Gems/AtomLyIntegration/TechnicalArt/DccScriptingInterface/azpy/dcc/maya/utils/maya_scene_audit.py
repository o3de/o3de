import config
from dynaconf import settings
from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtCore import Signal, Slot
from azpy.dcc.scene_audit_utility import SceneAuditor
from pathlib import Path
import logging
import sys


_LOGGER = logging.getLogger('DCCsi.azpy.dcc.maya.utils.maya_scene_audit')


class MayaSceneAuditor(SceneAuditor):
    """! DCC Application scene audit base class.

    Defines the tool's main window, messaging and loading systems and adds content widget
    """
    def __init__(self, target_application, target_files, operation='audit'):
        super(MayaSceneAuditor, self).__init__(target_application, target_files)

        self.target_application = target_application
        self.target_files = target_files
        self.operation = operation
        self.audit_data = {}
        self.animation_data = {}
        self.lighting_data = {}
        self.mesh_data = {}
        self.material_data = {}
        self.animation_data = {}
        self.camera_data = {}
        self.start_operation()

    def start_operation(self):
        _LOGGER.info(f'Maya Scene Auditor... Operation: {self.operation}   TargetFiles: {self.target_files}')
        for file in self.target_files:
            _LOGGER.info(f'Processing File: {file}')
            if self.operation == 'audit':
                self.audit_file()
            elif self.operation == 'track_object':
                _LOGGER.info('Track Object')

    def audit_file(self):
        _LOGGER.info('Audit')

    def track_object(self, target_object):
        pass

    def open_file(self, file_path):
        pass

    # +++++++++++++++++--->>>
    # GETTERS/SETTERS +---->>>
    # +++++++++++++++++--->>>

    def get_camera_information(self, target='all'):
        pass

    def get_mesh_information(self, target='all'):
        pass

    def get_lighting_information(self, target='all'):
        pass

    def get_material_information(self, target='all'):
        pass

    def get_animation_information(self, target='all'):
        pass


