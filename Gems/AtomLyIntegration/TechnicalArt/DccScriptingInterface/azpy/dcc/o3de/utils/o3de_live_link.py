from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtNetwork import QTcpSocket
from PySide2.QtCore import QDataStream
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.components as components
import azlmbr.legacy.general as general
import azlmbr.math as math
import logging
import json
import sys


_LOGGER = logging.getLogger('DCCsi.azpy.dcc.maya.utils.maya_live_link')


class O3DELiveLink(QtCore.QObject):
    """! Live link control system """

    def __init__(self, *args):
        super(O3DELiveLink, self).__init__()

        print(f'O3DE LiveLink fired!!!!!! Passed arg: {sys.argv}')
        self.dcc_scene_info = None
        self.level_name = None
        self.socket = QTcpSocket()
        self.socket.readyRead.connect(self.read_data)
        self.socket.connected.connect(self.connected)
        self.stream = None
        self.port = 17345

    def start_operation(self):
        print(f'Maya Live Link script firing ==> Operation: {self.operation}')
        self.add_callbacks()

    def establish_connection(self):
        try:
            self.socket.connectToHost('localhost', self.port)
        except Exception as e:
            _LOGGER.info(f'CONNECTION EXCEPTION [{type(e)}] :::: {e}')
            return False
        return True

    def connected(self):
        self.stream = QDataStream(self.socket)
        self.stream.setVersion(QDataStream.Qt_5_0)

    def read_data(self):
        msg = self.socket.readAll()
        converted = str(msg.data(), encoding='utf-8')
        try:
            message = json.loads(converted)
            _LOGGER.info(f'MESSAGE::::::::::: {message}')
        except Exception as e:
            _LOGGER.info(f'Error: [{e}]  --- Failed on this: {converted}')



if __name__ == '__main__':
    print('Script launch default function fired')
    O3DELiveLink(sys.argv)


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