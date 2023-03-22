from PySide2 import QtWidgets, QtGui, QtCore
from PySide2.QtWidgets import QPushButton, QWidget
from PySide2.QtGui import QPainter, QPainterPath, QPen, QColor, QIcon, QPixmap
from PySide2.QtCore import Qt, QRectF, Signal, Slot
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
        self.content_frame.setStyleSheet(
            f'background-image: url({background_image.as_posix()}); '
            f'background-repeat: no-repeat;'
        )

        self.buttons_layout = QtWidgets.QVBoxLayout()
        self.buttons_layout.setContentsMargins(25, 25, 25, 25)
        self.content_layout.addLayout(self.buttons_layout)

        # Getting started with O3DE
        self.o3de_button = SectionButton('Getting Started with O3DE', self.get_message(1), 3, 'rgb(0, 255, 255)',
                                         'startO3DE')
        self.o3de_button.clicked.connect(self.section_clicked)
        self.buttons_layout.addWidget(self.o3de_button)
        self.buttons_layout.addSpacing(10)

        # Getting started with the DCCsi
        self.dccsi_button = SectionButton('Getting Started with the DCCsi', self.get_message(2), 3,
                                          'rgb(0, 255, 255)', 'startDCCsi')
        self.dccsi_button.clicked.connect(self.section_clicked)
        self.buttons_layout.addWidget(self.dccsi_button)
        self.buttons_layout.addSpacing(10)

        # Launch Scripts
        self.launch_scripts_button = SectionButton('Launch Productivity Scripts', self.get_message(3), 3,
                                                   'rgb(0, 255, 255)', 'launchScripts')
        self.launch_scripts_button.clicked.connect(self.section_clicked)
        self.buttons_layout.addWidget(self.launch_scripts_button)
        self.buttons_layout.addSpacing(10)

        # Launch Content
        self.launch_scripts_button = SectionButton('Launch Content', self.get_message(4), 3,
                                                   'rgb(0, 255, 255)', 'launchContent')
        self.launch_scripts_button.clicked.connect(self.section_clicked)
        self.buttons_layout.addWidget(self.launch_scripts_button)

        self.get_info()

    def get_info(self):
        _LOGGER.info('Gathering Information for the Splash Section...')
        self.set_info('Splash Page... provide a description of the DCCsi and ways to leverage it. Also add information '
                      'for latest tools and functionality')

    def get_message(self, target_message):
        # might be good to eventually control this with data from the database that can be easily updated or changed
        message = None
        if target_message == 1:
            message = 'If you are new to O3DE Engine, click here to discover a collection of tips and tutorials to ' \
                      'help you better understand how to get the most out of the Editor and how to get started on ' \
                      'your first project.'
        elif target_message == 2:
            message = 'If you have never worked with the DCCsi before, take a look at some of the content contained' \
                      'here that will explain how this scripting framework can boost your productivity. It will ' \
                      'change the way that you approach projects.'
        elif target_message == 3:
            message = 'You may be looking for workflow scripts to maximize your efficiency, or ways to get to the fun '\
                      'of making games and simulations more quickly. We have a number of scripted tools ready for you' \
                      'to use now. Check them out here.'
        else:
            message = 'There is a catalog of sample content available that you can load right into the engine to begin'\
                      'experimenting with. Look here to browse what is available, and gain access to this content.'

        return message

    def set_info(self, info_text: str):
        # self.info.setText(info_text)
        pass

    def open_section(self):
        pass

    def close_section(self):
        pass

    @Slot(str)
    def section_clicked(self, target_section):
        _LOGGER.info(f'Target Section: {target_section}')


class SectionButton(QWidget):
    clicked = Signal(str)

    def __init__(self, title, message, border_size, title_color, section):
        super(SectionButton, self).__init__()

        self.border_size = border_size
        self.title_color = title_color
        self.outline_color = QColor(27, 105, 221, 255)
        self.section = section
        self.setFixedSize(500, 125)

        self.main_container = QtWidgets.QHBoxLayout(self)
        self.main_container.setContentsMargins(0, 0, 0, 0)
        self.content_container = QtWidgets.QVBoxLayout()
        self.main_container.addLayout(self.content_container)
        self.arrow_container = QtWidgets.QVBoxLayout()
        self.arrow_container.setContentsMargins(0, 0, 20, 0)
        self.main_container.addLayout(self.arrow_container)

        arrow_path = settings.get('PATH_DCCSI_TOOLS') / 'Launcher/images/arrow.png'
        arrow_pixmap = QtGui.QPixmap(arrow_path.as_posix())
        self.arrow_label = QtWidgets.QLabel()
        self.arrow_label.setPixmap(arrow_pixmap)
        self.arrow_container.addWidget(self.arrow_label)

        self.content_container.setContentsMargins(20, 20, 20, 20)
        self.content_container.setAlignment(QtCore.Qt.AlignTop)

        self.bold_font = QtGui.QFont('Helvetica [Cronyx]', 11, QtGui.QFont.Bold)
        self.regular_font = QtGui.QFont('Helvetica', 9, QtGui.QFont.Normal)

        self.title_label = QtWidgets.QLabel(title)
        self.title_label.setStyleSheet(f'color: {self.title_color};')
        self.title_label.setFont(self.bold_font)
        self.content_container.addWidget(self.title_label)
        self.message_text = QtWidgets.QTextEdit(message)
        block_format = QtGui.QTextBlockFormat()
        block_format.setLineHeight(125, QtGui.QTextBlockFormat.ProportionalHeight)
        cursor = self.message_text.textCursor()
        cursor.clearSelection()
        cursor.select(QtGui.QTextCursor.Document)
        cursor.mergeBlockFormat(block_format)
        self.message_text.setStyleSheet('QTextEdit { color: white; border: none; padding-bottom: 5px;}')
        self.message_text.setReadOnly(True)
        self.message_text.setFont(self.regular_font)
        self.message_text.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.message_text.viewport().setAutoFillBackground(False)
        self.content_container.addWidget(self.message_text)
        # self.click_label = QtWidgets.QLabel(' Click Here')
        # self.click_label.setStyleSheet('color: rgb(27, 105, 221);')
        # self.content_container.addWidget(self.click_label)
        self.setProperty('index', 1)
        self.installEventFilter(self)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing)
        path = QPainterPath()
        pen = QPen(self.outline_color, self.border_size)
        painter.setPen(pen)

        rect = QRectF(event.rect())
        rect.adjust(self.border_size/2, self.border_size/2, -self.border_size/2, -self.border_size/2)
        path.addRoundedRect(rect, 10, 10)
        painter.setClipPath(path)

        grad = QtGui.QLinearGradient(0, 0, 0, 100)
        grad.setColorAt(0.0, QColor(255, 255, 255, 60))
        grad.setColorAt(1.0, QColor(150, 150, 150, 30))
        painter.fillRect(rect, grad)
        painter.strokePath(path, painter.pen())

    def eventFilter(self, obj: QtWidgets.QLabel, event: QtCore.QEvent) -> bool:
        if isinstance(obj, QtWidgets.QWidget):
            if event.type() == QtCore.QEvent.MouseButtonPress:
                self.clicked.emit(self.section.lower())
            elif event.type() == QtCore.QEvent.Enter:
                self.setCursor(QtGui.QCursor(QtCore.Qt.PointingHandCursor))
            elif event.type() == QtCore.QEvent.Leave:
                self.setCursor(QtGui.QCursor(QtCore.Qt.ArrowCursor))
        return super(SectionButton, self).eventFilter(obj, event)
