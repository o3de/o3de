from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtCore import Signal, Slot
from dynaconf import settings
from pathlib import Path
import logging
import os

_LOGGER = logging.getLogger('Tools.Dev.Windows.Launcher.navigation')


class Navigation(QtWidgets.QWidget):
    section_requested = Signal(str)

    def __init__(self, model):
        super(Navigation, self).__init__()

        self.model = model
        self.setFixedHeight(75)
        self.launcher_sections = self.model.get_sections()
        self.navigation_frame = QtWidgets.QFrame(self)
        self.navigation_frame.setStyleSheet('background-color: rgb(27, 105, 221);')
        self.navigation_frame.setGeometry(0, 0, 10000, 90)
        self.main_container = QtWidgets.QHBoxLayout(self)

        # Top Banner Logo
        self.logo_container = QtWidgets.QHBoxLayout()
        self.main_container.addLayout(self.logo_container)
        self.logo_container.setAlignment(QtCore.Qt.AlignLeft)
        logo_path = settings.get('PATH_DCCSI_TOOLS') / 'Launcher/images/o3de_banner_logo.png'
        logo_pixmap = QtGui.QPixmap(logo_path.as_posix())
        self.o3de_logo = QtWidgets.QLabel()
        self.o3de_logo.setPixmap(logo_pixmap)
        self.logo_container.addWidget(self.o3de_logo)
        self.logo_container.addSpacing(4)
        self.launcher_title = QtWidgets.QLabel('Launcher')
        self.title_font = QtGui.QFont("Helvetica", 25, QtGui.QFont.Bold)
        self.launcher_title.setFont(self.title_font)
        self.logo_container.addWidget(self.launcher_title)

        # Top Banner Combobox
        self.combobox_container = QtWidgets.QHBoxLayout()
        self.main_container.addLayout(self.combobox_container)
        self.combobox_container.setAlignment(QtCore.Qt.AlignRight)
        self.combobox_container.addSpacing(15)
        self.navigation_combobox = QtWidgets.QComboBox()
        self.navigation_combobox.setStyleSheet('')
        self.navigation_combobox.setFixedSize(150, 25)

        self.navigation_combobox.addItems(self.launcher_sections)
        # self.navigation_combobox.setCurrentIndex(self.launcher_sections.index('Splash'))
        self.navigation_combobox.currentIndexChanged.connect(self.navigation_event)
        self.combobox_container.addWidget(self.navigation_combobox)

    def navigation_event(self):
        self.section_requested.emit(self.navigation_combobox.currentText().lower())

