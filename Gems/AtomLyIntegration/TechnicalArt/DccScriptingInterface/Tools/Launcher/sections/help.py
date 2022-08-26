import config
from PySide2 import QtWidgets, QtCore
# from PySide2 import Qw
from pathlib import Path
from mistletoe import Document, HTMLRenderer
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
        self.markdown_file = r"E:\Depot\o3de-engine\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\Tools\Launcher\markdown\PycharmDebugging\pycharm_debugging.md"
        self.text_field = QtWidgets.QTextEdit()
        self.content_layout.addWidget(self.text_field)
        self.set_info()

    def set_info(self):
        with open(self.markdown_file, 'r') as mdf:
            with HTMLRenderer() as renderer:
                doc = Document(mdf)
                self.text_field.setText(renderer.render(doc))

    # def set_info(self, target_file: QtCore.QUrl):
    # self.html_view.setUrl(target_file)
    # self.test_url = f'E:/Depot/o3de_dccsi/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/KitbashConverter/html/main_8py.html'
    # self.html_file = None


    def open_section(self):
        pass
        # self.html_file = QtCore.QUrl.fromLocalFile(self.test_url)
        # self.set_info(self.html_file)
        # self.content_layout.addWidget(self.html_view)

    def close_section(self):
        _LOGGER.info('Close section in help firing')
        # self.html_view.setUrl('')
        pass
