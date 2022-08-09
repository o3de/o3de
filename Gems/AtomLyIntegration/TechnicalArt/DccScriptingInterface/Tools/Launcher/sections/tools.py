from PySide2 import QtWidgets
import logging

_LOGGER = logging.getLogger('Launcher.tools')


class Tools(QtWidgets.QWidget):
    def __init__(self, model):
        super(Tools, self).__init__()

        _LOGGER.info('Tools Page added to content layout')
        self.model = model
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(0, 0, 0, 0)
        self.content_frame = QtWidgets.QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)
        self.content_frame.setStyleSheet('background-color:rgb(100, 0, 0);')
        self.info = QtWidgets.QTextEdit()
        self.content_layout.addWidget(self.info)
        self.get_info()

    def get_info(self):
        _LOGGER.info('Gathering Information for the Tools Section...')
        info_text = 'Tools Information:\n\nFound tools:\n'
        for index, value in self.model.tools.items():
            info_text += f'-- {index}: {value}\n'
        self.set_info(info_text)

    def set_info(self, info_text: str):
        self.info.setText(info_text)

    def open_section(self):
        pass

    def close_section(self):
        pass

