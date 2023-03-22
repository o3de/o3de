from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtCore import Signal, Slot
import logging

_LOGGER = logging.getLogger('Launcher.projects')


class Tutorials(QtWidgets.QWidget):
    info_bar_message = Signal(str)

    def __init__(self, model):
        super(Tutorials, self).__init__()

        self.model = model
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(0, 0, 0, 0)
        self.content_frame = QtWidgets.QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)
        self.content_frame.setStyleSheet('background-color:rgb(0, 0, 0);')

        self.info = QtWidgets.QTextEdit()
        self.content_layout.addWidget(self.info)
        self.get_info()

    def get_info(self):
        pass

    def set_info(self, info_text: str):
        pass

    def open_section(self):
        pass

    def close_section(self):
        pass
