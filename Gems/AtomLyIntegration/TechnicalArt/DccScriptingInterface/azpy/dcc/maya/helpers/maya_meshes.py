import maya.cmds as mc


def get_scene_objects():
    mesh_dict = {}
    mesh_objects = mc.ls(type='mesh')
    for target_mesh in mesh_objects:
        transform_node = mc.listRelatives(target_mesh, parent=True)[0]
        mesh_attributes = get_mesh_attributes(transform_node)
        mesh_dict[transform_node] = {
            'translation':   mesh_attributes['center'],
            'rotation':   mesh_attributes['rotate'],
            'scale': mesh_attributes['scale'],
            'objectColorRGB': mesh_attributes['objectColorRGB'],
            'visibility': mesh_attributes['visibility']
        }
    return mesh_dict


def get_mesh_attributes(target_mesh):
    target_attributes = ['scale', 'center', 'rotate', 'objectColorRGB', 'visibility']
    mesh_attributes = {}
    for attr in mc.listAttr(target_mesh):
        if attr in target_attributes:
            attr_value = mc.getAttr(f'{target_mesh}.{attr}')
            if isinstance(attr_value, list):
                if isinstance(attr_value, tuple):
                    attr_value = []
                    for count, axis in enumerate(['X', 'Y', 'Z']):
                        attr_value.append(attr_value[0][axis])
            mesh_attributes[attr] = attr_value
    return mesh_attributes
