import maya.cmds as mc
import logging


_MODULENAME = 'azpy.dcc.maya.helpers.maya_cameras'
_LOGGER = logging.getLogger(_MODULENAME)


def get_camera_info(target='all'):
    camera_dict = {}
    scene_cameras = get_all_cameras() if target == 'all' else [target]
    for camera in scene_cameras:
        focal_length = mc.camera(camera, q=True, fl=True)
        near_clip_plane = mc.camera(camera, q=True, ncp=True)
        far_clip_plane = mc.camera(camera, q=True, fcp=True)
        camera_transforms = get_transform_data(camera)

        camera_dict[camera] = {
            'position': camera_transforms['rotation'],
            'rotation': camera_transforms['position'],
            'near_clip_plane': near_clip_plane,
            'far_clip_plane': far_clip_plane,
            'focal_length': focal_length
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
    camera_position = mc.camera(target_camera, q=True, p=True)
    transform_node = mc.listRelatives(target_camera, parent=True)
    rotation_values = mc.getAttr(f'{transform_node[0]}.rotate')
    return {'position': camera_position, 'rotation': rotation_values}

