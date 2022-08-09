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
        logo_path = settings.get('PATH_DCCSI_TOOLS_DEV') / 'Launcher/images/o3de_banner_logo.png'
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
        self.navigation_combobox.setCurrentIndex(self.launcher_sections.index('Setup'))
        self.navigation_combobox.currentIndexChanged.connect(self.navigation_event)
        self.combobox_container.addWidget(self.navigation_combobox)

    def navigation_event(self):
        self.section_requested.emit(self.navigation_combobox.currentText().lower())

        # self.button_container = QtWidgets.QGridLayout()
        # self.section_buttons = {}
        # self.setLayout(self.button_container)
        # self.button_container.setContentsMargins(0, 0, 0, 0)
        # self.button_container.setSpacing(2)

        # for index, section in enumerate(self.model.get_sections()):
        #     section_button = NavigationButton(section)
        #     section_button.clicked.connect(self.navigation_button_clicked)
        #     self.section_buttons[section] = section_button
        #     self.button_container.addWidget(section_button, 0, index)

    # @Slot(str)
    # def navigation_button_clicked(self, target_section):
    #     self.section_requested.emit(target_section)
    #     for key, value in self.section_buttons.items():
    #         if key.lower() == target_section:
    #             value.set_button_state('active')
    #         else:
    #             value.set_button_state('enabled')


# class NavigationButton(QtWidgets.QWidget):
#     clicked = Signal(str)
#
#     def __init__(self, section):
#         super(NavigationButton, self).__init__()
#
#         self.section = section
#         self.active = False
#         self.button_frame = QtWidgets.QFrame(self)
#         self.button_frame.setGeometry(0, 0, 1000, 1000)
#         self.button_frame.setStyleSheet('background-color:rgb(35, 35, 35);')
#         self.hit = QtWidgets.QLabel(self.section)
#         self.hit.setStyleSheet('color: white')
#         self.hit.setAlignment(QtCore.Qt.AlignCenter)
#         self.hit.setProperty('index', 1)
#         self.hit.setFixedHeight(60)
#         self.hit.installEventFilter(self)
#         self.layout = QtWidgets.QVBoxLayout(self)
#         self.layout.setContentsMargins(0, 0, 0, 0)
#         self.layout.addWidget(self.hit)
#
#     def set_button_state(self, state: str):
#         if state == 'active':
#             _LOGGER.info(f'{self.section} button... set to {state}')
#             self.hit.setStyleSheet('font-weight: bold; color: red')
#             self.active = True
#         else:
#             self.hit.setStyleSheet('font-weight: regular; color: white;')
#             self.active = False
#
#     def eventFilter(self, obj: QtWidgets.QLabel, event: QtCore.QEvent) -> bool:
#         if isinstance(obj, QtWidgets.QWidget):
#             if event.type() == QtCore.QEvent.MouseButtonPress:
#                 self.clicked.emit(self.section.lower())
#             elif event.type() == QtCore.QEvent.Enter:
#                 if not self.active:
#                     self.hit.setStyleSheet('font-weight: bold; color: white;')
#             elif event.type() == QtCore.QEvent.Leave:
#                 if not self.active:
#                     self.hit.setStyleSheet('font-weight: regular; color: white;')
#         return super(NavigationButton, self).eventFilter(obj, event)
