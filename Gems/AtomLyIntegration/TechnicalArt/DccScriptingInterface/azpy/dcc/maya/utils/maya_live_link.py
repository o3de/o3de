from PySide2 import QtCore
from PySide2.QtCore import QDataStream
from PySide2.QtNetwork import QTcpSocket
import logging
import json


import maya.cmds as mc
from functools import partial


_LOGGER = logging.getLogger('DCCsi.azpy.dcc.maya.utils.maya_live_link')
SIZEOF_UINT32 = 4


class MayaLiveLink(QtCore.QObject):
    """! Live link control system """

    def __init__(self, **kwargs):
        super(MayaLiveLink, self).__init__()

        self.operation = kwargs['operation']
        self.scene_data = self.filter_scene_data(kwargs)
        self.header_size = 10
        self.tracked_items = {}
        self.tracked_attributes = [
            'translateX',
            'translateY',
            'translateZ',
            'rotateX',
            'rotateY',
            'rotateZ',
            'scaleX',
            'scaleY',
            'scaleZ'
        ]

        self.socket = QTcpSocket()
        self.socket.connected.connect(self.connected)
        self.socket.readyRead.connect(self.read_data)
        self.stream = None
        self.port = 17344

    def establish_connection(self):
        try:
            self.socket.connectToHost('localhost', self.port)
        except Exception as e:
            _LOGGER.info(f'CONNECTION EXCEPTION [{type(e)}] :::: {e}')
            return False
        return True

    def start_operation(self):
        self.establish_connection()
        self.add_callbacks()

    def connected(self):
        self.stream = QDataStream(self.socket)
        self.stream.setVersion(QDataStream.Qt_5_0)

    def filter_scene_data(self, data):
        filtered_dict = {}
        filter_values = ['class', 'target_application', 'target_files', 'operation']
        for key, values in data.items():
            if key not in filter_values:
                filtered_dict[key] = values
        return filtered_dict

    def add_callbacks(self):
        for key, values in self.scene_data.items():
            for k, v in values.items():
                # Track mesh translate/rotate/scale
                if k == 'meshes':
                    for item_name, item_values in v.items():
                        self.tracked_items[item_name] = {}
                        for attribute in self.tracked_attributes:
                            job = mc.scriptJob(attributeChange=[
                                f'{item_name}.{attribute}', partial(self.attribute_changed, [item_name, attribute])])
                            self.tracked_items[item_name].update({attribute: job})

    def attribute_changed(self, target):
        attr_value = mc.getAttr(f'{target[0]}.{target[1]}')
        update_values = [target[0], target[1], attr_value]
        self.send({'cmd': 'update_event', 'text': update_values})

    def send(self, cmd):
        json_cmd = json.dumps(cmd)
        if self.socket.waitForConnected(5000):
            self.stream << json_cmd
            self.socket.flush()
        else:
            _LOGGER.info('Connection to the server failed')
        return None

    def remove_callbacks(self):
        for key, values in self.tracked_items.items():
            for attribute, job_number in values.items():
                mc.scriptJob(kill=job_number, force=True)
