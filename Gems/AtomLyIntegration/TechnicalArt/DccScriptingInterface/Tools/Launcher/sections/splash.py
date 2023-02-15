from PySide2 import QtWidgets, QtGui
import logging
from dynaconf import settings

_LOGGER = logging.getLogger('Launcher.splash')


class Splash(QtWidgets.QWidget):
    def __init__(self, model):
        super(Splash, self).__init__()

        self.model = model
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(0, 0, 0, 0)
        self.content_frame = QtWidgets.QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)
        self.dccsi_path = settings.get('PATH_DCCSIG')

        background_image = settings.get('PATH_DCCSIG') / 'Tools/Launcher/images/splash_page.jpg'
        self.content_frame.setStyleSheet(f'background-image: url({background_image.as_posix()})')
        self.get_info()

    def get_info(self):
        _LOGGER.info('Gathering Information for the Splash Section...')
        self.set_info('Splash Page... provide a description of the DCCsi and ways to leverage it. Also add information '
                      'for latest tools and functionality')

    def set_info(self, info_text: str):
        # self.info.setText(info_text)
        pass

    def open_section(self):
        pass

    def close_section(self):
        pass
