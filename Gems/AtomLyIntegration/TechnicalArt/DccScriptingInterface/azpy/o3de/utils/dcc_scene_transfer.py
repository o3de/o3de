from PySide2 import QtCore
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.components as components
import azlmbr.legacy.general as general
import azlmbr.math as math
import logging
import json
import sys


_LOGGER = logging.getLogger('DCCsi.azpy.dcc.o3de.utils.o3de_maya_scene_transfer')


class DccSceneTransfer(QtCore.QObject):
    """! Live link control system """

    def __init__(self, **kwargs):
        super(DccSceneTransfer, self).__init__()

        self.scene_data = kwargs['scene_data']

    def start_operation(self):
        print('DCCSceneTransfer building scene...')
        _LOGGER.info('Logger comment')
