import config
from dynaconf import settings
from Tools.Launcher.main import ContentContainer
from Tools.Launcher.data.configuration import Configuration
from Tools.Launcher.data.model import LauncherModel
from PySide2 import QtWidgets, QtCore, QtGui
from pathlib import Path
import logging
import sys


_LOGGER = logging.getLogger('DCCsi.start')
_LOGGER.info('Start session')


class Launcher(QtWidgets.QMainWindow):
    """! The Launcher base class.

    Defines the tool's main window, messaging and loading systems and adds content widget
    """
    def __init__(self):
        super(Launcher, self).__init__()

        self.app = QtWidgets.QApplication.instance()
        self.setWindowFlags(QtCore.Qt.Window)
        self.setGeometry(50, 50, 1280, 720)
        self.setObjectName('DCCsiLauncher')
        self.setWindowTitle('DCCsi Launcher')
        self.icon_path = (settings.get('PATH_DCCSI_TOOLS') / 'Launcher/images/o3de_icon.png').as_posix()
        self.setWindowIcon(QtGui.QIcon(self.icon_path))
        self.isTopLevel()
        self.model = LauncherModel()
        self.configuration = Configuration()
        self.content = ContentContainer(self.model)
        self.setCentralWidget(self.content)

        for target_dict in self.model.tables.values():
            self.configuration.set_variables({k: v for k, v in target_dict.items()})

        self.status_bar = QtWidgets.QStatusBar()
        self.status_bar.setVisible(False)
        self.status_bar.setStyleSheet('background-color: rgb(35, 35, 35)')
        self.setStatusBar(self.status_bar)
        self.load_bar = QtWidgets.QProgressBar()
        self.load_bar.setFixedSize(200, 20)
        self.status_bar.addPermanentWidget(self.load_bar)
        self.status_bar.showMessage('Ready.')


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    with open(settings.get('GLOBAL_QT_STYLESHEET'), 'r') as stylesheet:
        app.setStyleSheet(stylesheet.read())

    launcher = Launcher()
    launcher.show()
    sys.exit(app.exec_())

