import maya.cmds as mc


def get_lighting_info():
    lights_dict = {}
    scene_lights = mc.ls(type='light')
    for target_light in scene_lights:
        light_type = mc.nodeType(target_light)
        transform_node = mc.listRelatives(target_light, parent=True)[0]
        light_transforms = get_light_transforms(target_light)
        light_attributes = get_light_attributes(target_light, light_type)

        lights_dict[transform_node] = {
            'type': light_type,
            'position': light_transforms['position'],
            'rotation': light_transforms['rotation'],
            'attributes': light_attributes
        }
    return lights_dict


def get_light_transforms(target_light):
    transform_node = mc.listRelatives(target_light, parent=True)
    mc.select(transform_node, r=True)
    light_position = mc.xform(sp=True, q=True, ws=True)
    rotation_values = mc.getAttr(f'{transform_node[0]}.rotate')[0]
    return {'position': light_position, 'rotation': rotation_values}


def get_light_attributes(target_light, light_type):
    light_attributes = {}
    target_attributes = ['color', 'intensity']
    if light_type == 'spotLight':
        target_attributes.extend(['coneAngle', 'penumbraAngle'])

    for attr in mc.listAttr(target_light):
        if attr in target_attributes:
            attr_value = mc.getAttr(f'{target_light}.{attr}')
            if isinstance(attr_value, list):
                attr_value = attr_value[0]
            light_attributes[attr] = attr_value
    return light_attributes
