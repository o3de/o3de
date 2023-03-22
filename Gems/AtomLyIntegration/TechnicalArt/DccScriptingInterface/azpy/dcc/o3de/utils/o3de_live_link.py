from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtNetwork import QTcpSocket
from PySide2.QtCore import QDataStream
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.components as components
import azlmbr.legacy.general as general
import math as standard_math
import azlmbr.math as math
import azlmbr.entity
import logging
import json
import sys


_LOGGER = logging.getLogger('DCCsi.azpy.dcc.o3de.utils.o3de_live_link')


class O3DELiveLink(QtCore.QObject):
    """! Live link control system """

    def __init__(self, **kwargs):
        super(O3DELiveLink, self).__init__()

        self.operation = kwargs['operation']
        self.kwargs = kwargs
        self.dcc_scene_info = None
        self.level_name = None
        self.socket = QTcpSocket()
        self.socket.readyRead.connect(self.read_data)
        self.socket.connected.connect(self.connected)
        self.stream = None
        self.port = 17345

    def start_operation(self):
        self.establish_connection()
        if self.operation == 'live-link':
            self.add_callbacks()
            return {'msg': 'live_link_ready', 'result': 'live_link_ready'}
        elif self.operation == 'scene-update':
            print(f"SceneUpdate: {self.kwargs['update_info']}")
            self.handle_scene_update(self.kwargs['update_info'])

    def establish_connection(self):
        try:
            self.socket.connectToHost('localhost', self.port)
        except Exception as e:
            print(f'CONNECTION EXCEPTION [{type(e)}] :::: {e}')
            return False
        return True

    def connected(self):
        self.stream = QDataStream(self.socket)
        self.stream.setVersion(QDataStream.Qt_5_0)

    def handle_scene_update(self, update_info):
        entity_id = self.get_entity(update_info[0])
        if update_info[1] == 'rotate':
            self.set_rotation(entity_id, update_info[-1][0])
        elif update_info[1] == 'translate':
            self.set_translation(entity_id, update_info[-1][0])
        elif update_info[1] == 'scale':
            self.set_scale(entity_id, update_info[-1][0])

    def get_updated_values(self, positional_values, target_index, value):
        for index, element in enumerate(positional_values):
            if index == target_index:
                positional_values[index] = value
                return positional_values

    def get_target_index(self, target_axis):
        target_index = {'X': 0, 'Y': 1, 'Z': 2}
        return target_index[target_axis]

    def set_translation(self, entity_id, pos):
        components.TransformBus(bus.Event, "SetWorldTranslation", entity_id, math.Vector3(pos[0], pos[1], pos[2]))
        general.idle_wait_frames(1)

    def set_rotation(self, entity_id, data):
        new_translation = math.Vector3(float(self.get_radians(data[0])), float(self.get_radians(data[1])),
                                       float(self.get_radians(data[2])))
        components.TransformBus(bus.Event, "SetLocalRotation", entity_id, new_translation)
        general.idle_wait_frames(1)

    def set_scale(self, entity_id, data):
        components.TransformBus(bus.Event, "SetLocalUniformScale", entity_id, data[0])
        general.idle_wait_frames(1)

    def attribute_changed(self, target):
        print(f'Attribute changed: {target}')

    def send(self, cmd):
        json_cmd = json.dumps(cmd)
        if self.socket.waitForConnected(5000):
            self.stream << json_cmd
            self.socket.flush()
        else:
            print('Connection to the server failed')
        return None

    def read_data(self):
        print('Read data...')

    def add_callbacks(self):
        pass

    @staticmethod
    def get_entity(entity_name):
        search_filter = azlmbr.entity.SearchFilter()
        search_filter.names = [entity_name]
        entity_object_id = azlmbr.entity.SearchBus(bus.Broadcast, 'SearchEntities', search_filter)[0]
        return entity_object_id

    @classmethod
    def get_radians(cls, degrees):
        return standard_math.radians(degrees)


# Good automation information here:
# https://wiki.agscollab.com/display/lmbr/AZ+Entity%2C+Component%2C+Property+Automation+API

# # opens a level but could prompt the user
# azlmbr.legacy.general.open_level(strLevelName)
#
# # opens a level without prompting the user (better for automation)
# azlmbr.legacy.general.open_level_no_prompt(strLevelName)
#
# # creates a level with the parameters of 'levelName', 'resolution', 'unitSize' and 'bUseTerrain'
# azlmbr.legacy.general.create_level(levelName, resolution, unitSize, bUseTerrain)
#
# # same as create_level() but no prompts
# azlmbr.legacy.general.create_level_no_prompt(levelName, resolution, unitSize, bUseTerrain)
#
# # saves the current level
# azlmbr.legacy.general.save_level()


# TJ's Code /////////////////////////////////////////////////////////////////////////////////////////////////


# """
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
# """
# # -------------------------------------------------------------------------
# """blender3d\\editor\\scripts\\blender3d_dialog.py
# Generated from O3DE PythonToolGem Template"""
#
# import azlmbr.bus as bus
# import azlmbr.components as components
# import azlmbr.legacy.general as general
# import azlmbr.editor as editor
# import azlmbr.entity as entity
# from azlmbr.entity import EntityId
# import azlmbr.math as math
# import sys
# import os
# import WhiteBoxInit as wbInit
# import azlmbr.whitebox.api as api
# import time
#
# from azlmbr.entity import EntityType
#
# from datetime import datetime
# from multiprocessing.connection import Client, Listener
#
# from PySide2.QtCore import Qt, QTimer
# from PySide2.QtGui import QDoubleValidator
# from PySide2.QtWidgets import QCheckBox, QComboBox, QDialog, QLineEdit, QFormLayout, QGridLayout, QGroupBox, QLabel, \
#     QPushButton, QHBoxLayout, QVBoxLayout, QWidget
#
# PORT_NUMBER = 7777
# PASSWORD = b'password'
#
# ADDRESS = ('192.168.4.39', PORT_NUMBER)
#
# O3DE_CHANNEL = "O3DE"
# BLENDER_CHANNEL = "BLENDER"
#
# test_cube_verts = [
#     [1, 1, 1],  # 0
#     [1, 1, 0],  # 1
#     [1, 0, 1],  # 2
#     [1, 0, 0],  # 3
#     [0, 1, 1],  # 4
#     [0, 1, 0],  # 5
#     [0, 0, 1],  # 6
#     [0, 0, 0],  # 7
# ]
#
# test_cube_faces = [
#     [0, 1, 2],
#     [1, 3, 2],
#     [3, 6, 2],
#     [3, 7, 6],
#     [7, 4, 6],
#     [7, 5, 4]
#     # [0, 4, 5],
#     # [6, 4, 5]
# ]
#
# # live_link_entity = None
# live_link_entity_mapping = dict()
# live_link_entity_reverse_mapping = dict()
# live_link_enabled = True
#
# # before_loc = [0,0,0]
# # before_rot = [0,0,0]
# # before_scale = 0
#
# conn = None
#
# reset_connection_time = 9 * 1000
# reset_reference_timestamp = 0
#
#
# def are_vecs_equal(a, b, epsilon=0.000001):
#     return abs(a[0] - b[0]) < epsilon and abs(a[1] - b[1]) < epsilon and abs(a[2] - b[2]) < epsilon
#
#
# def searchForEntitiesByName(name):
#     searchfilter = entity.SearchFilter()
#     searchfilter.names = [name]
#     return entity.SearchBus(bus.Broadcast, 'SearchEntities', searchfilter)
#
#
# def onWhiteboxUpdate():
#     print("Detected change to whitebox")
#
#
# class BlenderLiveLinkPythonDialog(QDialog):
#
#     def send_to_blender(self, cmd):
#         self.blender_send_queue.append(cmd)
#
#     # command functions
#     def set_value(self, args):
#         print("setting our value...")
#         self.value = args[0]
#
#     def init_blender_live_sync(self, args):
#         global live_link_entity_mapping
#         for entity_name, live_link_item in live_link_entity_mapping.items():
#             # get the mesh data
#             entity = live_link_item["id"]
#             whiteboxComponentIds = azlmbr.editor.EditorComponentAPIBus(bus.Broadcast,
#                                                                        'FindComponentTypeIdsByEntityType',
#                                                                        ['White Box'], EntityType().Game)
#             whiteboxComponentOutcome = azlmbr.editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', entity,
#                                                                            whiteboxComponentIds[0])
#             whiteboxComponent = whiteboxComponentOutcome.GetValue()
#             whiteboxMesh = azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(bus.Event,
#                                                                                          'GetWhiteBoxMeshHandle',
#                                                                                          whiteboxComponent)
#
#             self.whiteboxMeshChangeHandler.connect(entity)
#             vertlist, facelist = self.exportMeshData(whiteboxMesh)
#             position = components.TransformBus(bus.Event, "GetWorldTranslation", entity)
#             pos = [position.x, position.y, position.z]
#             rotation = components.TransformBus(bus.Event, "GetLocalRotation", entity)
#             rot = [rotation.x, rotation.y, rotation.z]
#             scale = components.TransformBus(bus.Event, "GetLocalUniformScale", entity)
#             self.send_to_blender(['SYNC_LIVE_MESH', 'INIT_MODEL', entity_name, pos, vertlist, facelist])
#             self.send_to_blender(['SYNC_LIVE_MESH', 'ROT', entity_name, rot])
#             self.send_to_blender(['SYNC_LIVE_MESH', 'SCALE', entity_name, scale])
#
#     def addEntityToLiveLink(self):
#         global live_link_entity_mapping
#         name = self.name_input.text()
#         if name in live_link_entity_mapping: return
#         candidates = searchForEntitiesByName(name)
#         if len(candidates) == 0:
#             print("ERROR: No entities by '{0}' could be found!".format(name))
#             return
#         if len(candidates) > 1:
#             print("ERROR: Too many entries with name '{0}'! ".format(name))
#             return
#         else:
#             # check that it has a whitebox component
#             entity = candidates[0]
#             whiteboxComponentIds = azlmbr.editor.EditorComponentAPIBus(bus.Broadcast,
#                                                                        'FindComponentTypeIdsByEntityType',
#                                                                        ['White Box'], EntityType().Game)
#             whiteboxComponentOutcome = azlmbr.editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', entity,
#                                                                            whiteboxComponentIds[0])
#             print(whiteboxComponentOutcome)
#
#             if whiteboxComponentOutcome.IsSuccess():
#                 whiteboxComponent = whiteboxComponentOutcome.GetValue()
#                 whiteboxMesh = azlmbr.whitebox.request.bus.EditorWhiteBoxComponentRequestBus(bus.Event,
#                                                                                              'GetWhiteBoxMeshHandle',
#                                                                                              whiteboxComponent)
#                 vertlist, facelist = self.exportMeshData(whiteboxMesh)
#                 position = components.TransformBus(bus.Event, "GetWorldTranslation", entity)
#                 pos = [position.x, position.y, position.z]
#                 rotation = components.TransformBus(bus.Event, "GetLocalRotation", entity)
#                 rot = [rotation.x, rotation.y, rotation.z]
#                 scale = components.TransformBus(bus.Event, "GetLocalUniformScale", entity)
#                 self.whiteboxMeshChangeHandler.connect(entity)
#
#                 # we can add this in
#                 live_link_entity_mapping[name] = {
#                     "id":           entity,
#                     "name":         name,
#                     "before_loc":   pos,
#                     "before_rot":   rot,
#                     "before_scale": scale
#                 }
#                 self.name_input.clear()
#                 self.send_to_blender(['SYNC_LIVE_MESH', 'INIT_MODEL', name, pos, vertlist, facelist])
#                 self.send_to_blender(['SYNC_LIVE_MESH', 'ROT', name, rot])
#                 self.send_to_blender(['SYNC_LIVE_MESH', 'SCALE', name, scale])
#                 self.refreshLiveLinkEntityList()
#
#             else:
#                 print("ERROR: '{0}' is not a whitebox mesh!".format(name))
#
#     def removeEntityFromLiveLink(self):
#         global live_link_entity_mapping
#         name = self.name_input.text()
#         live_link_entity_mapping.pop(name)
#         self.refreshLiveLinkEntityList()
#         self.send_to_blender(['SYNC_LIVE_MESH', 'UNLINK', name])
#         self.name_input.clear()
#
#     def sync_live_mesh(self, args):
#         global live_link_entity_mapping, live_link_entity_reverse_mapping
#         mode = args[0]
#         name = args[1]
#
#         sub_args = args[2:]
#         if mode == 'INIT_MODEL':
#             # the model does not exist yet in the scene, we need to create it
#             position = sub_args[0]
#             vertlist = sub_args[1]
#             facelist = sub_args[2]
#
#             # prevent feedback
#             self.createNewMesh(starting_name=name, blender_name=name, starting_position=position,
#                                starting_verts=vertlist, starting_tris=facelist, send_to_blender_flag=False)
#         elif mode == 'UPDATE_MODEL':
#             live_link_item = live_link_entity_mapping[live_link_entity_reverse_mapping[name]]
#             entity = live_link_item["id"]
#             vertlist = sub_args[0]
#             facelist = sub_args[1]
#             self.updateMesh(name=name, vertlist=vertlist, facelist=facelist)
#         elif mode == "POS":
#             live_link_item = live_link_entity_mapping[live_link_entity_reverse_mapping[name]]
#             entity_id = live_link_item["id"]
#             position = sub_args[0]
#             # prevent feedback
#             live_link_item['before_loc'] = position
#             new_pos = math.Vector3(position[0], position[1], position[2])
#             self.updateLiveLinkProperty(components.TransformBus, "SetWorldTranslation", entity_id, new_pos)
#         elif mode == "ROT":
#             live_link_item = live_link_entity_mapping[live_link_entity_reverse_mapping[name]]
#             entity_id = live_link_item["id"]
#             rotation = sub_args[0]
#             # prevent feedback
#             live_link_item['before_rot'] = rotation
#             # note: this is using local rotation, but the blender coords are likely world rotation... will need to update later
#             new_rot = math.Vector3(float(rotation[0]), float(rotation[1]), float(rotation[2]))
#             self.updateLiveLinkProperty(components.TransformBus, "SetLocalRotation", entity_id, new_rot)
#         elif mode == "SCALE":
#             live_link_item = live_link_entity_mapping[live_link_entity_reverse_mapping[name]]
#             entity_id = live_link_item["id"]
#             scale = sub_args[0]
#             # prevent feedback
#             live_link_item['before_scale'] = scale
#             self.updateLiveLinkProperty(components.TransformBus, "SetLocalUniformScale", entity_id, scale)
#
#     def connectToServer(self):
#         global conn, reset_reference_timestamp
#         self.conn = Client(ADDRESS, authkey=PASSWORD)
#         self.live_link_connect_button.setEnabled(False)
#         self.live_link_disconnect_button.setEnabled(True)
#         self.live_link_widget.setEnabled(True)
#         self.live_link_widget.setVisible(True)
#         self.server_timer.start()
#         conn = self.conn
#         self.resetAllChannels()
#         reset_reference_timestamp = datetime.now()
#
#     def disconnectFromServer(self):
#         global conn
#         self.conn.send('close')
#         self.conn = None
#         self.live_link_connect_button.setEnabled(True)
#         self.live_link_disconnect_button.setEnabled(False)
#         self.live_link_widget.setEnabled(False)
#         self.live_link_widget.setVisible(False)
#         self.server_timer.stop()
#         conn = None
#
#     # these functions are only accessible from a widget which is activated only when we have a valid connection
#     def pingServer(self):
#         self.conn.send(['PRINT_SENDER', 'o3de', 'hi there!'])
#         print(self.conn.recv())
#
#     def printServerStructs(self):
#         self.conn.send(['PRINT_STRUCTS'])
#         print(self.conn.recv())
#
#     def updateLiveLinkProperty(self, dest_bus, function_name, entity, value):
#         if entity is None: return
#         dest_bus(bus.Event, function_name, entity, value)
#
#     def determineBlenderUpdates(self):
#         global live_link_entity_mapping
#
#         for entity_name_key, live_link_item in live_link_entity_mapping.items():
#             entity = live_link_item["id"]
#             before_loc = live_link_item["before_loc"]
#             before_rot = live_link_item['before_rot']
#             before_scale = live_link_item['before_scale']
#
#             position = components.TransformBus(bus.Event, "GetWorldTranslation", entity)
#             rotation = components.TransformBus(bus.Event, "GetLocalRotation", entity)
#             scale = components.TransformBus(bus.Event, "GetLocalUniformScale", entity)
#
#             potential_new_position = [position.x, position.y, position.z]
#             potential_new_rotation = [rotation.x, rotation.y, rotation.z]
#
#             if not are_vecs_equal(before_loc, potential_new_position):
#                 live_link_item["before_loc"] = potential_new_position
#                 # we moved from our end, so send the update to blender
#                 self.send_to_blender(['SYNC_LIVE_MESH', 'POS', entity_name_key, potential_new_position])
#
#             if not are_vecs_equal(before_rot, potential_new_rotation):
#                 live_link_item["before_rot"] = potential_new_rotation
#                 self.send_to_blender(['SYNC_LIVE_MESH', 'ROT', entity_name_key, potential_new_rotation])
#
#             if before_scale != scale:
#                 live_link_item["before_scale"] = scale
#                 self.send_to_blender(['SYNC_LIVE_MESH', 'SCALE', entity_name_key, scale])
#
#         while len(self.blender_send_queue) != 0:
#             new_cmd = self.blender_send_queue.pop(0)
#             self.conn.send(['ENQUEUE_CHANNEL', BLENDER_CHANNEL, new_cmd])
#
#     def liveLinkUpdate(self):
#         global reset_reference_timestamp, reset_connection_time, conn
#         if not live_link_enabled: return
#
#         # first check if we need to reset our connection (we reset the connection to make sure time delay is refreshed)
#         new_time = datetime.now()
#         timeElapsed = new_time - reset_reference_timestamp
#         if timeElapsed.total_seconds() * 1000 >= reset_connection_time:
#             reset_reference_timestamp = new_time
#             self.conn.send('close')
#             self.conn.close()
#             self.conn = None
#             time.sleep(.5)
#             self.conn = Client(ADDRESS, authkey=PASSWORD)
#             conn = self.conn
#
#         # here we check for any new commands from our command queue. If any, act on them
#         self.processCommand()
#
#         # relevant UI updates
#
#         # determine updates to send to Blender
#         self.determineBlenderUpdates()
#
#     def createNewCube(self):
#         self.createNewMesh(use_default_cube=True)
#
#     def loadMeshData(self, mesh, model_verts, model_tris):
#         verts = []
#         tris = []
#         for v in model_verts:
#             vertex = math.Vector3(float(v[0]), float(v[1]), float(v[2]))
#             verts.append(mesh.AddVertex(vertex))
#
#         for ti in model_tris:
#             tris.append(api.util_MakeFaceVertHandles(verts[ti[0]], verts[ti[1]], verts[ti[2]]))
#
#         for t in tris:
#             mesh.AddPolygon([t])
#
#         return mesh
#
#     def exportMeshData(self, mesh):
#         vertlist = []
#         facelist = []
#
#         vertexCount = mesh.MeshVertexCount()
#         for i in range(vertexCount):
#             vertHandle = azlmbr.object.construct('VertexHandle', i)
#             new_vert = mesh.VertexPosition(vertHandle)
#             vertlist.append([new_vert.x, new_vert.y, new_vert.z])
#
#         faceCount = mesh.MeshFaceCount()
#         for i in range(faceCount):
#             faceHandle = azlmbr.object.construct('FaceHandle', i)
#             new_face = mesh.FaceVertexHandles(faceHandle)
#             new_list = []
#             for f in new_face:
#                 new_list.append(f.Index())
#             facelist.append(new_list)
#
#         return (vertlist, facelist)
#
#     def refreshLiveLinkEntityList(self):
#         global live_link_entity_mapping
#
#         while self.live_link_list_layout.count():
#             child = self.live_link_list_layout.takeAt(0)
#             if child.widget():
#                 child.widget().deleteLater()
#         for name_key, item in live_link_entity_mapping.items():
#             new_label = QLabel(name_key + " --> " + str(item["id"].ToString()))
#             self.live_link_list_layout.addWidget(new_label)
#
#     def updateMesh(self, name, vertlist, facelist):
#         candidate = searchForEntitiesByName(name)[0]
#         whiteboxComponentIds = azlmbr.editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType',
#                                                                    ['White Box'], EntityType().Game)
#         whiteboxComponentOutcome = azlmbr.editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', candidate,
#                                                                        whiteboxComponentIds[0])
#         meshComponent = whiteboxComponentOutcome.GetValue()
#         mesh = wbInit.create_white_box_handle(meshComponent)
#
#         mesh.Clear()
#         mesh = self.loadMeshData(mesh, vertlist, facelist)
#
#         wbInit.update_white_box(mesh, meshComponent)
#         mesh.CalculateNormals()
#         mesh.CalculatePlanarUVs()
#         azlmbr.whitebox.request.bus.EditorWhiteBoxComponentModeRequestBus(bus.Event,
#                                                                           'MarkWhiteBoxIntersectionDataDirty',
#                                                                           meshComponent)
#
#     # we only handle triangle meshes for now
#     def createNewMesh(self, starting_name="LiveLinkTestCube", blender_name=None, starting_position=None,
#                       use_default_cube=False, starting_verts=None, starting_tris=None, send_to_blender_flag=True):
#         global live_link_entity_mapping, live_link_entity_reverse_mapping
#
#         if starting_position is None:
#             starting_position = [1.0, 3.0, 5.0]
#
#         index = 0
#         current_name = starting_name
#
#         # ensure name is unique
#         _entities_should_be_empty = searchForEntitiesByName(current_name)
#
#         while len(_entities_should_be_empty) != 0:
#             current_name = starting_name + str(index)
#             index += 1
#             _entities_should_be_empty = searchForEntitiesByName(current_name)
#
#         entity = wbInit.create_white_box_entity(current_name)
#         meshComponent = wbInit.create_white_box_component(entity)
#         mesh = wbInit.create_white_box_handle(meshComponent)
#         self.whiteboxMeshChangeHandler.connect(entity)
#         mesh.Clear()
#
#         if use_default_cube:
#             mesh.initializeAsUnitCube()
#         else:
#             model_verts = test_cube_verts
#             model_tris = test_cube_faces
#
#             if starting_verts is not None and starting_tris is not None:
#                 model_verts = starting_verts
#                 model_tris = starting_tris
#             mesh = self.loadMeshData(mesh, model_verts, model_tris)
#
#         wbInit.update_white_box(mesh, meshComponent)
#         mesh.CalculateNormals()
#         mesh.CalculatePlanarUVs()
#         azlmbr.whitebox.request.bus.EditorWhiteBoxComponentModeRequestBus(bus.Event,
#                                                                           'MarkWhiteBoxIntersectionDataDirty',
#                                                                           meshComponent)
#
#         # note: we have to manually search for entity id only because the original create entity function for whiteboxes earlier did not gaurantee the correct id name
#         entityIdList = searchForEntitiesByName(current_name)
#         current_entity = entityIdList[0]
#
#         print("entities found: " + str(entityIdList))
#         print(entityIdList[0].ToString())
#
#         components.TransformBus(bus.Event, "SetWorldTranslation", current_entity,
#                                 math.Vector3(starting_position[0], starting_position[1], starting_position[2]))
#
#         new_vec = components.TransformBus(bus.Event, "GetWorldTranslation", current_entity)
#
#         # now we will attempt to get the mesh
#         vertlist, facelist = self.exportMeshData(mesh)
#
#         new_position = [new_vec.x, new_vec.y, new_vec.z]
#
#         if send_to_blender_flag:
#             self.send_to_blender(['SYNC_LIVE_MESH', 'INIT_MODEL', current_name, new_position, vertlist, facelist])
#
#         # add a new mapping
#         live_link_entity_mapping[current_name] = {
#             "id":           current_entity,
#             "name":         current_name,
#             "before_loc":   new_position,
#             "before_rot":   [0, 0, 0],
#             "before_scale": 1
#         }
#
#         self.refreshLiveLinkEntityList()
#         if type(blender_name) == type(""):
#             live_link_entity_reverse_mapping[blender_name] = current_name
#         else:
#             live_link_entity_reverse_mapping[current_name] = current_name
#
#     def processCommand(self):
#         if self.conn is None: return
#         self.conn.send(['DEQUEUE_CHANNEL', O3DE_CHANNEL])
#         command = self.conn.recv()
#
#         if type(command) == type([]) and command[0] in self.o3de_command_table:
#             print("This is our command: " + str(command))
#             command_name = command[0]
#             command_args = command[1:]
#             self.o3de_command_table[command_name](command_args)
#
#     def closeEvent(self, evnt):
#         global conn
#         if self.conn is not None:
#             self.conn.send('close')
#             self.conn.close()
#             self.conn = None
#             conn = None
#             self.server_timer.stop()
#         super(BlenderLiveLinkPythonDialog, self).closeEvent(evnt)
#
#     def resetAllChannels(self):
#         self.conn.send(["CLEAR_CHANNEL", O3DE_CHANNEL])
#         self.conn.recv()
#         self.conn.send(['CLEAR_CHANNEL', BLENDER_CHANNEL])
#         self.conn.recv()
#
#     def print_o3de(self, args):
#         print(" || ".join([str(x) for x in args]))
#
#     def printToBlender(self):
#         self.send_to_blender(['PRINT', 'THIS PING IS FROM O3DE', datetime.now()])
#
#     def __init__(self, parent=None):
#         global reset_reference_timestamp
#         super(BlenderLiveLinkPythonDialog, self).__init__(parent)
#
#         self.o3de_command_table = {
#             "SET_VALUE":      self.set_value,
#             "SYNC_LIVE_MESH": self.sync_live_mesh,
#             "PRINT":          self.print_o3de,
#             "INIT_BLENDER":   self.init_blender_live_sync
#         }
#
#         self.blender_send_queue = []
#
#         main_layout = QVBoxLayout()
#         main_layout.setSpacing(20)
#
#         self.conn = None
#
#         # now for the live linking buttons
#         connect_layout = QHBoxLayout()
#
#         self.live_link_connect_button = QPushButton("Connect")
#         self.live_link_connect_button.clicked.connect(self.connectToServer)
#
#         self.live_link_disconnect_button = QPushButton("Disconnect")
#         self.live_link_disconnect_button.clicked.connect(self.disconnectFromServer)
#         self.live_link_disconnect_button.setEnabled(False)
#
#         connect_layout.addWidget(self.live_link_connect_button)
#         connect_layout.addWidget(self.live_link_disconnect_button)
#
#         main_layout.addLayout(connect_layout)
#
#         self.live_link_layout = QVBoxLayout()
#         self.live_link_widget = QWidget()
#
#         test_line_widget = QGroupBox("Link Server Test!", self)
#         form_layout = QFormLayout()
#
#         self.name_input = QLineEdit(self)
#         self.name_input.setPlaceholderText("Enter in the name of Entity")
#         self.name_input.setClearButtonEnabled(True)
#         self.add_entity_button = QPushButton("+")
#         self.add_entity_button.clicked.connect(self.addEntityToLiveLink)
#         self.remove_entity_button = QPushButton("-")
#         self.remove_entity_button.clicked.connect(self.removeEntityFromLiveLink)
#
#         entityAddLayout = QHBoxLayout()
#         entityAddLayout.addWidget(self.name_input)
#         entityAddLayout.addWidget(self.add_entity_button)
#         entityAddLayout.addWidget(self.remove_entity_button)
#
#         eaddframe = QWidget()
#         eaddframe.setLayout(entityAddLayout)
#
#         self.name_input2 = QLineEdit(self)
#         self.name_input2.setPlaceholderText("Yo there!")
#         self.name_input2.setClearButtonEnabled(True)
#
#         form_layout.addRow("WhiteBox To Add", eaddframe)
#         form_layout.addRow("Other Line Edit", self.name_input2)
#         test_line_widget.setLayout(form_layout)
#         self.live_link_layout.addWidget(test_line_widget)
#
#         utility_box = QGroupBox("Utilities")
#         utility_layout = QVBoxLayout()
#
#         ping_button = QPushButton("Ping server")
#         ping_button.clicked.connect(self.pingServer)
#
#         print_button = QPushButton("Print Server Structs")
#         print_button.clicked.connect(self.printServerStructs)
#
#         process_button = QPushButton("Process Commands")
#         process_button.clicked.connect(self.processCommand)
#
#         clear_channels_button = QPushButton("Clear Channels")
#         clear_channels_button.clicked.connect(self.resetAllChannels)
#
#         utility_layout.addWidget(ping_button)
#         utility_layout.addWidget(print_button)
#         utility_layout.addWidget(process_button)
#         utility_layout.addWidget(clear_channels_button)
#
#         utility_box.setLayout(utility_layout)
#
#         blender_box = QGroupBox("Blender")
#         blender_layout = QVBoxLayout()
#
#         create_cube_button = QPushButton("Create new Cube")
#         create_cube_button.clicked.connect(self.createNewCube)
#
#         print_blender_button = QPushButton("Print To Blender")
#         print_blender_button.clicked.connect(self.printToBlender)
#
#         blender_layout.addWidget(create_cube_button)
#         blender_layout.addWidget(print_blender_button)
#
#         blender_box.setLayout(blender_layout)
#
#         live_link_list_box = QGroupBox("Current Live Linked Entities")
#         self.live_link_list_layout = QVBoxLayout()
#         self.live_link_list_layout.addWidget(QLabel("---"))
#         live_link_list_box.setLayout(self.live_link_list_layout)
#
#         self.live_link_layout.addWidget(utility_box)
#         self.live_link_layout.addWidget(blender_box)
#         self.live_link_layout.addWidget(live_link_list_box)
#
#         self.live_link_widget.setLayout(self.live_link_layout)
#         self.live_link_widget.setEnabled(False)
#         self.live_link_widget.setVisible(False)
#         main_layout.addWidget(self.live_link_widget)
#
#         main_layout.addStretch()
#
#         self.value = 0
#         self.server_timer = QTimer()
#         self.server_timer.setInterval(50)
#         self.server_timer.timeout.connect(self.liveLinkUpdate)
#
#         self.setLayout(main_layout)
#
#         self.whiteboxMeshChangeHandler = azlmbr.bus.NotificationHandler("EditorWhiteBoxComponentNotificationBus")
#         print(self.whiteboxMeshChangeHandler)
#
#         self.whiteboxMeshChangeHandler.add_callback('OnWhiteBoxMeshModified', onWhiteboxUpdate)
#
#         reset_reference_timestamp = datetime.now()
#
#
# if __name__ == "__main__":
#     # Create a new instance of the tool if launched from the Python Scripts window,
#     # which allows for quick iteration without having to close/re-launch the Editor
#     test_dialog = BlenderLiveLinkPythonDialog()
#     test_dialog.setWindowTitle("BlenderLiveLinkPython")
#     test_dialog.show()
#     test_dialog.adjustSize()