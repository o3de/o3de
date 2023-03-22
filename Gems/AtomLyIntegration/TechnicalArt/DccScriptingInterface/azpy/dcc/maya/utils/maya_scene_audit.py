import maya.cmds as mc
import maya.mel as mel
import logging
from PySide2.QtCore import Signal, Slot
from importlib import reload
from azpy.dcc.scene_audit_utility import SceneAuditor
from azpy.dcc.maya.helpers import maya_materials
from azpy.dcc.maya.helpers import maya_cameras
from azpy.dcc.maya.helpers import maya_lighting
from azpy.dcc.maya.helpers import maya_meshes
import json
import sys
import os


from PySide2.QtCore import QDataStream
from PySide2.QtNetwork import QTcpSocket


_LOGGER = logging.getLogger('DCCsi.azpy.dcc.maya.utils.maya_scene_audit')


class MayaSceneAuditor(SceneAuditor):
    """! DCC Application scene audit base class.

    Defines the tool's main window, messaging and loading systems and adds content widget
    """
    def __init__(self, **kwargs):
        super(MayaSceneAuditor, self).__init__()

        print(kwargs)
        # self.target_file = kwargs['target_file']
        self.up_axis = mc.upAxis(q=True, axis=True)
        self.scene_units = mc.currentUnit(q=True)
        self.debug_mode = True
        self.audit_data = {}
        self.animation_data = {}
        self.lighting_data = {}
        self.mesh_data = {}
        self.material_data = {}
        self.animation_data = {}
        self.camera_data = {}

        self.socket = QTcpSocket()
        self.socket.connected.connect(self.connected)
        self.stream = None
        self.port = 17344

        self.start_operation()

    def establish_connection(self):
        try:
            self.socket.connectToHost('localhost', self.port)
        except Exception as e:
            _LOGGER.info(f'CONNECTION EXCEPTION [{type(e)}] :::: {e}')
            return False
        return True

    def connected(self):
        self.stream = QDataStream(self.socket)
        self.stream.setVersion(QDataStream.Qt_5_0)

    def start_operation(self):
        self.reload_imports()
        self.get_scene_info()

        return {'msg': 'audit_complete', 'result': self.audit_data}

    def send(self, cmd):
        json_cmd = json.dumps(cmd)
        if self.socket.waitForConnected(5000):
            self.stream << json_cmd
            self.socket.flush()
        else:
            _LOGGER.info('Connection to the server failed')
        return None

    def clear_data(self):
        self.audit_data = {}
        self.animation_data = {}
        self.lighting_data = {}
        self.mesh_data = {}
        self.material_data = {}
        self.animation_data = {}
        self.camera_data = {}

    def reload_imports(self):
        """
        Maya loads modules once only by default. This function ensures that when refactoring and debugging maya script
        functionality that Maya python is picking up changes.
        """
        if self.debug_mode:
            reload(maya_cameras)
            reload(maya_lighting)
            reload(maya_meshes)
            reload(maya_materials)

    def create_file_list(self, file_list):
        if isinstance(file_list, str):
            current_file = mc.file(q=True, sn=True)
            return [current_file]
        return file_list

    def get_scene_info(self, object_filter=None):
        temp_dict = {'up_axis': self.up_axis, 'scene_units': self.scene_units}
        if object_filter is None:
            object_filter = ['cameras', 'lights', 'meshes', 'materials']
        if 'cameras' in object_filter:
            temp_dict.update({'cameras': self.get_camera_information()})
        if 'lights' in object_filter:
            temp_dict.update({'lights': self.get_lighting_information(self.up_axis)})
        if 'meshes' in object_filter:
            temp_dict.update({'meshes': self.get_mesh_information()})
        if 'materials' in object_filter:
            temp_dict.update({'materials': self.get_material_information(temp_dict['meshes'])})
        temp_dict.update({'scene_file': mc.file(q=True, sn=True)})
        self.audit_data = {'cmd': 'audit', 'scene_data': temp_dict}

    def track_object(self, target_object):
        pass

    def open_file(self, file_path):
        pass

    # +++++++++++++++++--->>>
    # GETTERS/SETTERS +---->>>
    # +++++++++++++++++--->>>

    def get_file_information(self, target_file):
        if target_file == self.current_file:
            return target_file
        try:
            self.current_file = mc.file(target_file, o=True, force=True)
            return target_file
        except RuntimeError:
            return None

    def get_camera_information(self, target='all'):
        return maya_cameras.get_camera_info()

    def get_lighting_information(self, target='all'):
        return maya_lighting.get_lighting_info()

    def get_mesh_information(self, target='all'):
        return maya_meshes.get_mesh_info()

    def get_material_information(self, mesh_list):
        mesh_list = [k for k, v in mesh_list.items()]
        return maya_materials.get_material_info(mesh_list)

    def get_animation_information(self, target='all'):
        pass

    # ++++++++++++++++--->>>
    # ERROR HANDLING +---->>>
    # ++++++++++++++++--->>>

    def handle_conversion_error(self, error):
        _LOGGER.info(f'Conversion error occurred [{type(error)}] ::::: {error}')
