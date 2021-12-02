"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

import socket
import pickle
import traceback
import sys

# BUFFER_SIZE = 4096
# port = 20201
#
# if len(sys.argv) > 1:
#     port = sys.argv[1]
#
# def SendCommand(target_command):
#     maya_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#     maya_socket.connect(('localhost', port))
#
#     try:
#         maya_socket.send("import maya.cmds as cmds".encode())
#         data = maya_socket.recv(BUFFER_SIZE)
#         maya_socket.send("cmds.polySphere()".encode())
#         data = maya_socket.recv(BUFFER_SIZE)
#
#         # This is a workaround for replacing "null bytes" and converting
#         # return information as a list (as opposed to a string)
#         result = eval(data.decode().replace('\x00', ''))
#         print(result[0])
#     except Exception as e:
#         print ('Connection Failed: {}'.format(e))
#     finally:
#         maya_socket.close()
#
#
#     # maya.send('import maya.cmds as mc; mc.polyCube()')
#     # maya.close()
#
# if __name__=='__main__':
#     target_command = "import maya.cmds as mc; mc.polySphere();"
#     SendCommand(target_command)




#Port Number 20201
# MayaVersion + 0 (Mel) or 1 (Python)
#
# import maya.cmds as mc
# mc.commandPort(name=":20201", sourceType="python")
#
#
# His user setup has this in it:
#
# if not mc.about(batch=True):
#     mc.commandPort(name=":20200", sourceType="mel")
#     mc.commandPort(name=":20201", sourceType="python")


class MayaClient(object):
    PORT = 20201
    BUFFER_SIZE = 4096

    def __init__(self):
        self.maya_socket = None
        self.port = self.__class__.PORT

    def connect(self, port=-1):
        if port >= 0:
            self.port = port
        try:
            self.maya_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.maya_socket.connect(('localhost', self.PORT))
        except:
            traceback.print_exc()
            return False
        return True

    def disconnect(self):
        try:
            self.maya_socket.close()
        except:
            traceback.print_exc()
            return False
        return True

    def send(self, cmd):
        try:
            self.maya_socket.sendall(cmd.encode())
        except:
            traceback.print_exc()
            return None
        return self.recv()

    def recv(self):
        try:
            data = self.maya_socket.recv(MayaClient.BUFFER_SIZE)
        except:
            traceback.print_exc()
            return None
        return data.decode().replace('\x00', '')

    ##################
    # COMMANDS   #####
    ##################

    def echo(self, text):
        cmd = "eval(\"'{}'\")".format(text)
        return self.send(cmd)

    def new_file(self):
        cmd = "cmds.file(new=True, force=True)"
        return self.send(cmd)

    def create_primitive(self, shape):
        cmd = ''
        if shape == 'sphere':
            cmd += 'cmds.polySphere()'
        elif shape == 'cube':
            cmd += 'cmds.polyCube()'
        else:
            print('Invalid Shape: {}'.format(shape))
            return None
        result = self.send(cmd)
        return eval(result)

    def translate(self, node, translation):
        cmd = "cmds.setAttr('{0}.translate', {1}, {2}, {3})".format(node, *translation)


if __name__ == '__main__':
    maya_client = MayaClient()
    if maya_client.connect():
        print('Connected successfully')
        print('Echo: {}'.format(maya_client.echo('hello world')))

        file_name = maya_client.new_file()
        print(file_name)

        nodes = maya_client.create_primitive('sphere')
        print(nodes)

        maya_client.translate(nodes[0], [0, 10, 0])

        nodes = maya_client.create_primitive('cube')
        print(nodes)


        if maya_client.disconnect():
            print('Disconnected successfully')
    else:
        print('Failed to connect')




if __name__ == "__main__":
    maya_client = MayaClient()
