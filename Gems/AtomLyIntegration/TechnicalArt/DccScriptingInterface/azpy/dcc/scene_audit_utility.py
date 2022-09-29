import config
from dynaconf import settings
from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtCore import QObject
import logging


_LOGGER = logging.getLogger('DCCsi.azpy.dcc.scene_audit_utility')


class SceneAuditor(QObject):
    """! DCC Application scene audit base class.

    Defines the tool's main window, messaging and loading systems and adds content widget
    """
    def __init__(self, target_application, target_files, operation='audit'):
        super(SceneAuditor, self).__init__()

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
        self.audit_started = False

    def start_audit(self):
        pass

    def process_file(self):
        pass

    def track_object(self, target_object):
        pass

    def open_file(self, file_path):
        pass

    def display_output(self, data):
        _LOGGER.info(f'{self.target_application} Audit Output:::> {data}')

    # +++++++++++++++++--->>>
    # GETTERS/SETTERS +---->>>
    # +++++++++++++++++--->>>

    def get_camera_information(self):
        pass

    def get_mesh_information(self):
        pass

    def get_lighting_information(self):
        pass

    def get_material_information(self):
        pass

    def get_animation_information(self):
        pass


