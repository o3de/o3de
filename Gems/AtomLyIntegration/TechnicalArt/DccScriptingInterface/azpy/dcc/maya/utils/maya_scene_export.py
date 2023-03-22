import maya.cmds as mc
import logging
from azpy.dcc.scene_audit_utility import SceneAuditor
from azpy.dcc.maya.helpers import maya_meshes
from azpy.o3de.renderer.materials import material_generator
from pathlib import Path
import shutil
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
        self.scene_info.pop('scene_file')
        self.operation = kwargs['operation']
        self.base_dccsi_location = kwargs['PATH_DCCSIG']
        self.base_repository_location = kwargs['O3DE_DEV']
        self.current_file = mc.file(q=True, sn=True)
        self.objects_directory = None
        self.export_data = None

    def start_operation(self):
        if self.operation == 'sceneExport':
            self.objects_directory = self.create_export_directory()
            export_items = self.export_mesh_files()
            return {'msg': 'asset_export_complete', 'result': export_items}

    def create_export_directory(self):
        export_directory = Path(self.current_file).stem
        objects_directory_path = Path(self.asset_output_location) / export_directory / 'Assets/Objects'
        objects_directory_path.mkdir(parents=True, exist_ok=True)
        return objects_directory_path

    def export_mesh_files(self):
        export_data = {}
        for key, values in self.scene_info.items():
            if key == 'meshes':
                for object_name, object_transforms in values.items():
                    mesh_export_directory = self.objects_directory / object_name
                    mesh_export_directory.mkdir(parents=True, exist_ok=True)
                    fbx_name = f'{object_name}.fbx'
                    mesh_export_path = mesh_export_directory / fbx_name
                    maya_meshes.export_fbx(object_name, mesh_export_path)
                    result = self.export_mesh_materials(object_name, mesh_export_directory, fbx_name)
                    if result:
                        export_data[object_name] = {'fbx': mesh_export_path.as_posix(), 'material': result}
        return export_data

    def export_mesh_materials(self, target_mesh, export_directory, fbx_name):
        for key, values in self.scene_info.items():
            if key == 'materials':
                for mesh_name, material_values in values.items():
                    if mesh_name == target_mesh:
                        material_name = None
                        attached_textures = None
                        for component_type, component_data in material_values.items():
                            if component_data:
                                if component_type == 'material_name':
                                    material_name = f'{Path(fbx_name).stem}_{component_data}.material'
                                else:
                                    attached_textures = self.export_texture_sets(component_data, export_directory)
                        return self.generate_material_description(material_name, attached_textures, export_directory)

    def generate_material_description(self, material_name, attached_textures, export_directory):
        if attached_textures:
            material_definition = material_generator.MaterialGenerator(
                material_type='StandardPBR',
                material_textures=attached_textures,
                material_name=material_name,
                start_directory=self.asset_output_location,
                destination_directory=export_directory,
                dccsi_path=self.base_dccsi_location,
                repo_path=self.base_repository_location).create_material_definition()
            return material_definition
        return None

    @staticmethod
    def export_texture_sets(texture_dictionary, export_directory):
        attached_textures = {}
        for texture_type, texture_values in texture_dictionary.items():
            if texture_values:
                for key, value in texture_values.items():
                    if key == 'path':
                        src = value
                        file_name = Path(value).name
                        target_export_directory = export_directory / 'textures'
                        target_export_directory.mkdir(parents=True, exist_ok=True)
                        dst = target_export_directory / file_name
                        attached_textures[texture_type] = dst.as_posix()
                        if not dst.exists():
                            shutil.copy(src, dst)
        return attached_textures
