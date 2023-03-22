import maya.cmds as mc
import logging


_MODULENAME = 'azpy.dcc.maya.helpers.maya_cameras'
_LOGGER = logging.getLogger(_MODULENAME)


def get_camera_info(target='all'):
    camera_dict = {}
    scene_cameras = get_all_cameras() if target == 'all' else [target]
    for camera in scene_cameras:
        camera_transforms = get_transform_data(camera)
        transform_node = mc.listRelatives(camera, parent=True)[0]

        camera_dict[transform_node] = {
            'translation': camera_transforms['position'],
            'rotation': camera_transforms['rotation'],
            'attributes': get_camera_attributes(camera)
        }
    return camera_dict


def get_all_cameras():
    default_cameras = ['frontShape', 'perspShape', 'sideShape', 'topShape']
    camera_list = []
    for camera in mc.ls(type='camera'):
        if camera not in default_cameras:
            camera_list.append(camera)
    return camera_list


def get_transform_data(target_camera):
    mc.select(target_camera, r=True)
    camera_position = mc.xform(sp=True, q=True, ws=True)
    transform_node = mc.listRelatives(target_camera, parent=True)
    rotation_values = mc.getAttr(f'{transform_node[0]}.rotate')[0]
    return {'position': camera_position, 'rotation': rotation_values}


def get_camera_attributes(camera):
    camera_attributes = {}
    focal_length = mc.camera(camera, q=True, fl=True)
    camera_attributes['focal_length'] = focal_length
    near_clip_plane = mc.camera(camera, q=True, ncp=True)
    camera_attributes['near_clip_distance'] = near_clip_plane
    far_clip_plane = mc.camera(camera, q=True, fcp=True)
    camera_attributes['far_clip_distance'] = far_clip_plane

    return camera_attributes
