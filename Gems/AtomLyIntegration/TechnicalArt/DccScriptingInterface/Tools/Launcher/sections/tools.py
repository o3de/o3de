from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtCore import Signal, Slot
from Tools.Launcher.sections import section_utilities as utilities
from Tools.Launcher.data import project_constants as constants
import logging

_LOGGER = logging.getLogger('Launcher.tools')


class Tools(QtWidgets.QWidget):
    def __init__(self, model):
        super(Tools, self).__init__()

        _LOGGER.info('Tools Page added to content layout')
        self.model = model
        self.tools_listings = self.model.tools
        self.current_section = None
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(10, 3, 10, 10)
        self.content_frame = QtWidgets.QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)

        self.add_button_group = QtWidgets.QButtonGroup()
        self.add_button_group.idClicked.connect(self.add_button_clicked)

        self.splitter_layout = QtWidgets.QVBoxLayout()
        self.content_layout.addLayout(self.splitter_layout)
        self.page_splitter = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        self.page_splitter.setSizes([150, 200, 565, 565])
        self.splitter_layout.addWidget(self.page_splitter)

        # +++++++++++++++++++++--->>>
        # LEFT COLUMN WIDGETS +---->>>
        # +++++++++++++++++++++--->>>

        self.left_column_widget = QtWidgets.QWidget()
        self.left_column_widget.setMaximumWidth(100)
        self.left_column_container = QtWidgets.QVBoxLayout()
        self.left_column_container.setContentsMargins(0, 0, 0, 0)
        self.left_column_widget.setLayout(self.left_column_container)
        self.page_splitter.addWidget(self.left_column_widget)

        # Tool Category Selection Panel ########################################
        self.tool_category_layout = QtWidgets.QVBoxLayout()
        self.tool_category_layout.setAlignment(QtCore.Qt.AlignTop)
        self.tool_category_panel = QtWidgets.QWidget()
        self.tool_category_panel.setLayout(self.tool_category_layout)
        self.tool_category_layout.setContentsMargins(0, 7, 0, 0)
        self.panel_frame = QtWidgets.QFrame(self.tool_category_panel)
        self.panel_frame.setStyleSheet('background-color: rgb(135, 135, 135);')
        self.panel_frame.setGeometry(0, 0, 150, 5000)
        self.left_column_container.addWidget(self.tool_category_panel)

        # --> Tool Category Button Layout
        self.category_button_container = QtWidgets.QVBoxLayout()
        self.category_button_container.setContentsMargins(10, 3, 10, 10)
        self.tool_category_layout.addLayout(self.category_button_container)

        self.section_buttons = {}
        self.section_button = None
        self.active_section_button = None
        for tool_type in self.model.tools:
            self.section_buttons.update({tool_type: self.add_section_button(tool_type)})
            self.category_button_container.addWidget(self.section_buttons[tool_type])

        # +++++++++++++++++++++++--->>>
        # CENTER COLUMN WIDGETS +---->>>
        # +++++++++++++++++++++++--->>>

        self.center_column_widget = QtWidgets.QWidget()
        self.center_column_widget.setFixedWidth(200)
        self.center_column_container = QtWidgets.QVBoxLayout()
        self.center_column_container.setContentsMargins(0, 0, 0, 0)
        self.center_column_widget.setLayout(self.center_column_container)
        self.page_splitter.addWidget(self.center_column_widget)

        # Tool Listings Panel ########################################
        self.tool_listings_layout = QtWidgets.QVBoxLayout()
        self.tool_listings_layout.setAlignment(QtCore.Qt.AlignTop)
        self.tool_listings_panel = QtWidgets.QWidget()
        self.tool_listings_panel.setLayout(self.tool_listings_layout)
        self.tool_listings_layout.setContentsMargins(0, 0, 0, 0)
        self.panel_frame = QtWidgets.QFrame(self.tool_listings_panel)
        self.panel_frame.setStyleSheet('background-color: rgb(50, 50, 50);')
        self.panel_frame.setGeometry(0, 0, 200, 5000)
        self.center_column_container.addWidget(self.tool_listings_panel)

        # +++++++++++++++++++++++--->>>
        # RIGHT COLUMN WIDGETS +---->>>
        # +++++++++++++++++++++++--->>>

        self.stacked_layout = QtWidgets.QStackedLayout()
        self.stacked_widget = QtWidgets.QWidget()
        self.stacked_widget.setLayout(self.stacked_layout)
        self.page_splitter.addWidget(self.stacked_widget)

        # Markdown display layer
        self.markdown_display = QtWidgets.QTextEdit()
        self.stacked_layout.addWidget(self.markdown_display)

        # Add Tool Layer
        self.add_tool_layout = QtWidgets.QVBoxLayout()
        # self.add_tool_layout.setAlignment(QtCore.Qt.AlignTop)
        self.add_tool_widget = QtWidgets.QWidget()
        self.add_tool_widget.setLayout(self.add_tool_layout)
        self.stacked_layout.addWidget(self.add_tool_widget)

        # Name
        self.tool_name_label = QtWidgets.QLabel('Name')
        self.add_tool_layout.addWidget(self.tool_name_label)
        self.tool_name_field = QtWidgets.QLineEdit()
        self.tool_name_field.setFixedHeight(25)
        self.add_tool_layout.addWidget(self.tool_name_field)

        # Category
        self.tool_category_label = QtWidgets.QLabel('Category')
        self.add_tool_layout.addWidget(self.tool_category_label)
        self.tool_category_combobox = QtWidgets.QComboBox()
        self.tool_category_combobox.setFixedHeight(25)
        self.tool_categories = (
            'Utility',
            'Workflow',
            'Conversion',
            'Add New Category'
        )
        self.add_tool_layout.addWidget(self.tool_category_combobox)
        self.tool_category_combobox.addItems(self.tool_categories)

        # Description
        self.tool_description_label = QtWidgets.QLabel('Description')
        self.add_tool_layout.addWidget(self.tool_description_label)
        self.tool_description_field = QtWidgets.QTextEdit()
        self.add_tool_layout.addWidget(self.tool_description_field)

        # Location
        self.tool_location_label = QtWidgets.QLabel('Start File Location')
        self.add_tool_layout.addWidget(self.tool_location_label)
        self.tool_location_layout = QtWidgets.QHBoxLayout()
        self.add_tool_layout.addLayout(self.tool_location_layout)
        self.tool_location_field = QtWidgets.QLineEdit()
        self.tool_location_field.setFixedHeight(25)
        self.tool_location_layout.addWidget(self.tool_location_field)
        self.tool_location_browse = QtWidgets.QPushButton('Browse')
        self.tool_location_browse.setFixedSize(75, 25)
        self.tool_location_layout.addWidget(self.tool_location_browse)

        # Tool Documentation
        self.tool_documentation_location = QtWidgets.QLabel('Documentation Location')
        self.add_tool_layout.addWidget(self.tool_documentation_location)
        self.tool_documentation_layout = QtWidgets.QHBoxLayout()
        self.add_tool_layout.addLayout(self.tool_documentation_layout)
        self.tool_documentation_field = QtWidgets.QLineEdit()
        self.tool_documentation_field.setFixedHeight(25)
        self.tool_documentation_layout.addWidget(self.tool_documentation_field)
        self.tool_documentation_browse = QtWidgets.QPushButton('Browse')
        self.tool_documentation_browse.setFixedSize(75, 25)
        self.tool_documentation_layout.addWidget(self.tool_documentation_browse)

        # Markdown Location
        self.tool_markdown_location = QtWidgets.QLabel('Markdown Location')
        self.add_tool_layout.addWidget(self.tool_markdown_location)
        self.markdown_location_layout = QtWidgets.QHBoxLayout()
        self.add_tool_layout.addLayout(self.markdown_location_layout)
        self.markdown_location_field = QtWidgets.QLineEdit()
        self.markdown_location_field.setFixedHeight(25)
        self.markdown_location_layout.addWidget(self.markdown_location_field)
        self.markdown_create_button = QtWidgets.QPushButton('Create New')
        self.markdown_create_button.setFixedSize(90, 25)
        self.markdown_location_layout.addWidget(self.markdown_create_button)
        self.markdown_location_browse = QtWidgets.QPushButton('Browse')
        self.markdown_location_browse.setFixedSize(75, 25)
        self.markdown_location_layout.addWidget(self.markdown_location_browse)
        self.add_tool_layout.addSpacing(15)

        # Add/Remove Tool Listing
        self.line = None
        self.separator_layout = self.get_separator_bar()
        self.add_tool_layout.addLayout(self.separator_layout)
        self.add_remove_button_layout = QtWidgets.QHBoxLayout()
        self.add_tool_layout.addLayout(self.add_remove_button_layout)

        # Add tool button
        self.add_tool_button = QtWidgets.QPushButton('Add Tool')
        self.add_tool_button.setObjectName('Primary')
        self.add_tool_button.setFixedHeight(50)
        self.add_tool_button.clicked.connect(self.add_tool_clicked)
        self.add_remove_button_layout.addWidget(self.add_tool_button)

        # Remove Tool Button
        self.remove_tool_button = QtWidgets.QPushButton('Remove Tool')
        self.remove_tool_button.setObjectName('Secondary')
        self.remove_tool_button.setFixedSize(100, 50)
        self.remove_tool_button.clicked.connect(self.remove_tool_clicked)
        self.add_remove_button_layout.addWidget(self.remove_tool_button)

        # ++++++++++++++++++++++++--->>>
        # BOTTOM CONTROL WIDGETS +---->>>
        # ++++++++++++++++++++++++--->>>

        self.bottom_control_widget = QtWidgets.QWidget()
        self.bottom_control_widget.setFixedHeight(100)
        self.bottom_control_layout = QtWidgets.QVBoxLayout()
        self.bottom_control_layout.setContentsMargins(0, 0, 0, 0)
        self.bottom_control_widget.setLayout(self.bottom_control_layout)
        self.panel_frame = QtWidgets.QFrame(self.bottom_control_widget)
        self.panel_frame.setStyleSheet('background-color: rgb(0, 0, 0);')
        self.panel_frame.setGeometry(0, 0, 5000, 100)
        self.content_layout.addWidget(self.bottom_control_widget)

        self.get_info()

    def get_info(self):
        _LOGGER.info('Gathering Information for the Tools Section...')
        for index, value in self.model.tools.items():
            _LOGGER.info(f'Tool: {index} ---> {value}')
        self.set_info()

    def add_section_button(self, section_name):
        self.section_button = QtWidgets.QPushButton(section_name)
        self.section_button.setFont(constants.BOLD_FONT)
        self.section_button.setCursor(QtGui.QCursor(QtCore.Qt.PointingHandCursor))
        self.section_button.setFixedHeight(50)
        self.section_button.setObjectName('Primary')
        self.section_button.clicked.connect(self.section_button_clicked)
        return self.section_button

    def get_tool_button(self, label_text):
        self.tool_button = QtWidgets.QPushButton()
        self.tool_button.setText(label_text)
        self.tool_button.setFixedSize(200, 35)
        self.tool_button.setStyleSheet("QPushButton{ border: 1px solid black; color: #00b4ef; background-color: "
                                  "qlineargradient(spread:pad, x1:1, y1:0, x2:1, y2:1, stop:0 rgba(100, 100, 100, 255),"
                                  " stop:1 rgba(60, 60, 60, 255));}")
        # TODO - Add rollover state
        self.tool_button.setFont(constants.BOLD_FONT)
        self.tool_button.clicked.connect(self.tool_button_clicked)
        return self.tool_button

    def validate_tool_information(self):
        _LOGGER.info('Validate tool information')

    def get_separator_bar(self):
        self.separator_layout = QtWidgets.QHBoxLayout()
        self.line = QtWidgets.QFrame()
        self.line.setStyleSheet('color: rgb(20, 20, 20);')
        self.line.setFrameShape(QtWidgets.QFrame.HLine)
        self.line.setLineWidth(1)
        self.separator_layout.addWidget(self.line)
        return self.separator_layout

    def set_section(self, button_name, target_button):
        self.close_section()
        target_button.setEnabled(False)
        self.active_section_button = target_button
        self.current_section = button_name
        self.set_tools(button_name)

    def set_tools(self, section_name):
        _LOGGER.info(f'Set tools fired: {section_name}')
        for i in reversed(range(self.tool_listings_layout.count())):
            self.tool_listings_layout.itemAt(i).widget().setParent(None)

        for i in range(len(self.tools_listings[section_name])):
            btn = self.get_tool_button(self.tools_listings[section_name][i])
            self.tool_listings_layout.addWidget(btn)

    def set_info(self):
        pass

    def open_section(self):
        """ Initializes Tools Window """
        pass

    def close_section(self):
        if self.active_section_button:
            self.active_section_button.setEnabled(True)

    # ++++++++++++++++--->>>
    # BUTTON ACTIONS +---->>>
    # ++++++++++++++++--->>>

    def add_button_clicked(self):
        _LOGGER.info('Add Button Clicked')

    def section_button_clicked(self):
        signal_sender = self.sender()
        button_name = signal_sender.text()
        self.set_section(button_name, self.section_buttons[button_name])

    def add_tool_clicked(self):
        _LOGGER.info('Add Tool Button Clicked')
        self.validate_tool_information()
        self.stacked_layout.setCurrentIndex(1)

    def remove_tool_clicked(self):
        _LOGGER.info('Remove Tool Button Clicked')

    def tool_button_clicked(self):
        signal_sender = self.sender()
        button_name = signal_sender.text()
        _LOGGER.info(f'CurrentSection: {self.current_section}  SelectedScript: {button_name}')



