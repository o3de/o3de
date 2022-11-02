import maya.cmds as mc


def get_lighting_info():
    lights_dict = {}
    scene_lights = mc.ls(type='light')
    for light in scene_lights:
        light_type = mc.nodeType(light)
        light_transforms = get_light_transforms(light)
        light_attributes = get_light_attributes(light, light_type)

        lights_dict[light] = {
            'type': light_type,
            'position': light_transforms['position'],
            'rotation': light_transforms['rotation'],
            'attributes': light_attributes
        }
    return lights_dict


def get_light_transforms(target_light):
    transform_node = mc.listRelatives(target_light, parent=True)
    light_position = mc.xform(transform_node, q=True, ws=True, ro=True)
    rotation_values = mc.getAttr(f'{transform_node[0]}.rotate')[0]
    return {'position': light_position, 'rotation': rotation_values}


def get_light_attributes(target_light, light_type):
    light_attributes = {}
    target_attributes = ['color', 'intensity']
    if light_type == 'spotLight':
        target_attributes.append('coneAngle')

    for attr in mc.listAttr(target_light):
        if attr in target_attributes:
            attr_value = mc.getAttr(f'{target_light}.{attr}')
            if isinstance(attr_value, list):
                attr_value = attr_value[0]
            light_attributes[attr] = attr_value
    return light_attributes
