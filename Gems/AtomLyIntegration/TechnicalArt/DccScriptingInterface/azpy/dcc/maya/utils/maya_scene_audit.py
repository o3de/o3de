import maya.cmds as mc
import logging
from PySide2.QtCore import Signal, Slot
from importlib import reload
from azpy.dcc.scene_audit_utility import SceneAuditor
from azpy.dcc.maya.helpers import maya_materials
from azpy.dcc.maya.helpers import maya_cameras
from azpy.dcc.maya.helpers import maya_lighting
from azpy.dcc.maya.helpers import maya_meshes
import json


_LOGGER = logging.getLogger('DCCsi.azpy.dcc.maya.utils.maya_scene_audit')


class MayaSceneAuditor(SceneAuditor):
    """! DCC Application scene audit base class.

    Defines the tool's main window, messaging and loading systems and adds content widget
    """
    def __init__(self, **kwargs):
        super(MayaSceneAuditor, self).__init__()

        self.target_application = kwargs['target_application']
        self.target_files = self.create_file_list(kwargs['target_files'])
        self.operation = kwargs['operation']
        self.current_file = mc.file(q=True, sn=True)
        self.debug_mode = True
        self.audit_data = {}
        self.animation_data = {}
        self.lighting_data = {}
        self.mesh_data = {}
        self.material_data = {}
        self.animation_data = {}
        self.camera_data = {}

    def start_operation(self):
        self.reload_imports()
        for file in self.target_files:
            target_file = self.get_file_information(file)
            if target_file:
                if self.operation == 'audit':
                    self.get_scene_info()
                    return self.audit_data
                elif self.operation == 'track_object':
                    _LOGGER.info('Track Object')

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
        temp_dict = {}
        if object_filter is None:
            object_filter = ['cameras', 'lights', 'meshes', 'materials']
        if 'cameras' in object_filter:
            temp_dict.update({'cameras': self.get_camera_information()})
        if 'lights' in object_filter:
            temp_dict.update({'lights': self.get_lighting_information()})
        if 'meshes' in object_filter:
            temp_dict.update({'meshes': self.get_mesh_information()})
        if 'materials' in object_filter:
            temp_dict.update({'materials': self.get_material_information(temp_dict['meshes'])})
        self.audit_data = {'cmd': 'audit', self.current_file: temp_dict}

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
        return maya_meshes.get_scene_objects()

    def get_material_information(self, mesh_list):
        mesh_list = [k for k, v in mesh_list.items()]
        return maya_materials.get_material_info(mesh_list)

    def get_animation_information(self, target='all'):
        pass

