from PySide2 import QtWidgets
import logging

_LOGGER = logging.getLogger('Launcher.splash')


class Splash(QtWidgets.QWidget):
    def __init__(self, model):
        super(Splash, self).__init__()

        _LOGGER.info('Splash Page added to content layout')
        self.model = model
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(0, 0, 0, 0)
        self.content_frame = QtWidgets.QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)
        self.content_frame.setStyleSheet('background-color:rgb(100, 0, 100);')
        self.info = QtWidgets.QTextEdit()
        self.content_layout.addWidget(self.info)
        self.get_info()

    def get_info(self):
        _LOGGER.info('Gathering Information for the Splash Section...')
        self.set_info('Splash Page... provide a description of the DCCsi and ways to leverage it. Also add information '
                      'for latest tools and functionality')

    def set_info(self, info_text: str):
        self.info.setText(info_text)

    def open_section(self):
        pass

    def close_section(self):
        pass
