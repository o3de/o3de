from PySide2 import QtWidgets
from dynaconf import settings
from pathlib import Path
import logging

_LOGGER = logging.getLogger('Launcher.output')


class Output(QtWidgets.QWidget):
    def __init__(self, model):
        super(Output, self).__init__()

        _LOGGER.info('Output Page added to content layout')
        self.model = model
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(0, 0, 0, 0)
        self.content_frame = QtWidgets.QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)
        self.content_frame.setStyleSheet('background-color:rgb(100, 100, 0);')
        self.log_file = Path(settings.DCCSI_LOG_PATH).parent / 'launcher.log'
        self.info = QtWidgets.QTextEdit()
        self.content_layout.addWidget(self.info)
        self.get_info()
        self.initialize_log()

    def initialize_log(self):
        open(self.log_file, 'w').close()

    def get_info(self):
        _LOGGER.info('Gathering Information for the Output Section...')
        self.set_info(str(self.model.get_table_names()))

    def set_info(self, info_text: str):
        self.info.setText(info_text)

    def update_log(self):
        log_messages = 'Log messages: \n\n'
        with open(self.log_file) as log:
            output = log.readlines()
            for line in output:
                log_messages += f'{line}\n'
            self.info.setText(log_messages)

    def open_section(self):
        self.update_log()

    def close_section(self):
        self.info.clear()
