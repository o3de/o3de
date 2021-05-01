import socket
import pickle
import traceback
import sys


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
            self.maya_socket.connect(('localhost', self.port))
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