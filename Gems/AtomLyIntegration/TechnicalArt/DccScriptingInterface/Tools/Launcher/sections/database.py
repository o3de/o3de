from PySide2 import QtWidgets
from dynaconf import settings
from pathlib import Path
import logging

_LOGGER = logging.getLogger('Launcher.database')


class Database(QtWidgets.QWidget):
    def __init__(self, model):
        super(Database, self).__init__()

        _LOGGER.info('Database Page added to content layout')
        self.model = model
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(0, 0, 0, 0)
        self.content_frame = QtWidgets.QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)
        self.content_frame.setStyleSheet('background-color:rgb(100, 100, 0);')
        self.info = QtWidgets.QTextEdit()
        self.content_layout.addWidget(self.info)
        self.get_info()

    def get_info(self):
        _LOGGER.info('Gathering Information for the Database Section...')
        # Retrieve database values here- use tables to allow easy way to change database values, backup data, etc

    def set_info(self, info_text: str):
        self.info.setText(info_text)

    def open_section(self):
        pass

    def close_section(self):
        pass
