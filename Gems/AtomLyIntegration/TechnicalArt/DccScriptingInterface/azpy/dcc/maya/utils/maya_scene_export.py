import maya.cmds as mc
import logging
from azpy.dcc.scene_audit_utility import SceneAuditor
from pathlib import Path
import json


_LOGGER = logging.getLogger('DCCsi.azpy.dcc.maya.utils.maya_scene_export')


class MayaSceneExporter(SceneAuditor):
    """! DCC Application scene audit base class.

    Defines the tool's main window, messaging and loading systems and adds content widget
    """
    def __init__(self, **kwargs):
        super(MayaSceneExporter, self).__init__()

        self.asset_output_location = kwargs['export_location']
        self.scene_info = kwargs['audit_info']
        self.operation = kwargs['operation']
        self.current_file = mc.file(q=True, sn=True)
        self.objects_directory = None

    def start_operation(self):
        if self.operation == 'sceneExport':
            _LOGGER.info('Start Export operation fired')
            _LOGGER.info(f'Output location: {self.asset_output_location}')
            _LOGGER.info(f'Audit Info: {self.scene_info}')
            self.objects_directory = self.create_export_directory()
            self.export_mesh_files()

    def create_export_directory(self):
        export_directory = Path(self.current_file).stem
        objects_directory_path = Path(self.asset_output_location) / export_directory / 'Assets/Objects'
        objects_directory_path.mkdir(parents=True, exist_ok=True)
        return objects_directory_path

    def export_mesh_files(self):
        for key, values in self.scene_info.items():
            for object_type, element_listings in values.items:
                if object_type == 'meshes':
                    for object_name, object_transforms in element_listings.items():
                        mesh_images_directory =

    def get_object_materials(self, object_materials):
        for key, values in self.scene_info.items():
            for object_type, element_listings in values.items:
                if object_type == 'materials':
                    for object_name, material_values in element_listings.items():





