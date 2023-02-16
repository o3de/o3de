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
        self.operation = kwargs['operation']
        self.base_dccsi_location = kwargs['PATH_DCCSIG']
        self.base_repository_location = kwargs['O3DE_DEV']
        self.current_file = mc.file(q=True, sn=True)
        self.objects_directory = None

    def start_operation(self):
        if self.operation == 'sceneExport':
            _LOGGER.info('Scene Export firing!')
            self.objects_directory = self.create_export_directory()
            self.export_mesh_files()

    def create_export_directory(self):
        export_directory = Path(self.current_file).stem
        objects_directory_path = Path(self.asset_output_location) / export_directory / 'Assets/Objects'
        objects_directory_path.mkdir(parents=True, exist_ok=True)
        return objects_directory_path

    def export_mesh_files(self):
        for key, values in self.scene_info.items():
            for object_type, element_listings in values.items():
                if object_type == 'meshes':
                    for object_name, object_transforms in element_listings.items():
                        mesh_export_directory = self.objects_directory / object_name
                        mesh_export_directory.mkdir(parents=True, exist_ok=True)
                        fbx_name = f'{object_name}.fbx'
                        mesh_export_path = mesh_export_directory / fbx_name
                        if not mesh_export_path.exists():
                            maya_meshes.export_fbx(object_name, mesh_export_path)
                            self.export_mesh_materials(object_name, mesh_export_directory, fbx_name)

    def export_texture_sets(self, texture_dictionary, export_directory):
        attached_textures = {}
        for texture_type, texture_values in texture_dictionary.items():
            _LOGGER.info(f'TextureType: {texture_type}  TextureValues: {texture_values}')
            if texture_values:
                for key, value in texture_values.items():
                    if key == 'path':
                        src = value
                        file_name = Path(value).name
                        target_export_directory = export_directory / 'textures'
                        target_export_directory.mkdir(parents=True, exist_ok=True)
                        dst = target_export_directory / file_name
                        attached_textures[texture_type] = dst
                        if not dst.exists():
                            shutil.copy(src, dst)
        return attached_textures

    def export_mesh_materials(self, target_mesh, export_directory, fbx_name):
        for key, values in self.scene_info.items():
            for object_type, element_listings in values.items():
                if object_type == 'materials':
                    for mesh_name, material_values in element_listings.items():
                        # _LOGGER.info(f'MeshName: {mesh_name}   MaterialValues: {material_values}')
                        if mesh_name == target_mesh:
                            material_name = None
                            attached_textures = None
                            for component_type, component_data in material_values.items():
                                if component_data:
                                    if component_type == 'material_name':
                                        material_name = f'{Path(fbx_name).stem}_{component_data}.material'
                                    else:
                                        attached_textures = self.export_texture_sets(component_data, export_directory)
                                self.generate_material_description(material_name, attached_textures, export_directory)

    def generate_material_description(self, material_name, attached_textures, export_directory):
        _LOGGER.info(f'Generate Material Description: [{material_name}] >>> {attached_textures} >>> {export_directory}')
        # material_definition = material_generator.MaterialGenerator(
        #     material_type='StandardPBR',
        #     material_properties=attached_textures,
        #     material_name=material_name,
        #     PATH_DCCSIG=self.base_dccsi_location,
        #     O3DE_DEV=self.base_repository_location).get_material()
        # return material_definition
