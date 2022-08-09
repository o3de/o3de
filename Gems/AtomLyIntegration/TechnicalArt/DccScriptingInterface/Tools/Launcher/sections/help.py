from PySide2 import QtWidgets, QtCore
# from PySide2 import Qw
from pathlib import Path
import logging
import os

_LOGGER = logging.getLogger('Launcher.help')


class Help(QtWidgets.QWidget):
    def __init__(self, model):
        super(Help, self).__init__()

        _LOGGER.info('Help Page added to content layout')
        self.model = model
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(0, 0, 0, 0)
        self.content_frame = QtWidgets.QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)
        self.content_frame.setStyleSheet('background-color:rgb(0, 100, 0);')
        # self.html_view = None
        self.test_url = f'E:/Depot/o3de_dccsi/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/KitbashConverter/html/main_8py.html'
        self.html_file = None
        # self.content_layout.addWidget(self.html_view)

    def set_info(self, target_file: QtCore.QUrl):
        # self.html_view.setUrl(target_file)
        pass

    def open_section(self):
        pass
        # self.html_file = QtCore.QUrl.fromLocalFile(self.test_url)
        # self.set_info(self.html_file)
        # self.content_layout.addWidget(self.html_view)

    def close_section(self):
        _LOGGER.info('Close section in help firing')
        # self.html_view.setUrl('')
        pass
