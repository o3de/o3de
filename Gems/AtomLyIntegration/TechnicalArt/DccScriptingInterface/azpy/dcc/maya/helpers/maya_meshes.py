import maya.cmds as mc


def get_scene_objects():
    mesh_objects = mc.ls(type='mesh')
    return mesh_objects