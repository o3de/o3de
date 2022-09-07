from PySide2 import QtGui

BOLD_FONT = QtGui.QFont("Helvetica", 7, QtGui.QFont.Bold)

BOLD_FONT_LARGE = QtGui.QFont("Helvetica", 8, QtGui.QFont.DemiBold)

BOLD_FONT_XTRA_LARGE = QtGui.QFont("Helvetica", 18, QtGui.QFont.DemiBold)

BOLD_FONT_SPLASH = QtGui.QFont("SansSerif", 25, QtGui.QFont.Bold)

PROJECTS_CATEGORIES = [
    'REGISTERED_PROJECTS',
    'REGISTERED_ENGINE',
    'BOOTSTRAP_LOCATION',
    'CURRENT_PROJECT',
    'CURRENT_ENGINE_PROJECTS',
    'CURRENT_ENGINE_GEMS',
    'ENGINE_EXTERNAL_DIRECTORIES',
    'O3DE_MANIFEST'
]

PROJECTS_HEADERS = [
    'projectName',
    'projectPath',
    'projectLevels'
]

TOOLS_CATEGORIES = [
    'O3DE',
    'Utilities',
    'Maya',
    'Blender',
    '3dsMax',
    'Painter',
    'Designer',
    'Houdini',
    'SAT'
]

TOOLS_HEADERS = [
    'toolId',
    'toolSection',
    'toolPlacement',
    'toolName',
    'toolDescription',
    'toolStartFile',
    'toolCategory',
    'toolMarkdown',
    'toolDocumentation'
]

TAB_STYLESHEET = 'QTabBar::tab { ' \
'background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, ' \
'stop: 0 #E1E1E1, stop: 0.4 #DDDDDD, ' \
'stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3); ' \
'border: 2px solid #C4C4C3; ' \
'border-bottom-color: #C2C7CB;' \
'border-top-left-radius: 4px; ' \
'border-top-right-radius: 4px; ' \
'min-width: 8ex; ' \
'padding: 2px;' \
'color: rgb(135, 135, 135);' \
'}' \
'QTabBar::tab:selected, QTabBar::tab:hover {' \
'background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,' \
'stop: 0  #fafafa, stop: 0.4 #f4f4f4,' \
'stop: 0.5  #e7e7e7, stop: 1.0 #fafafa);' \
'color: #FF00FF;' \
'}' \
'QTabBar::tab:selected {' \
'border-color: #9B9B9B;' \
'border-bottom-color:  #C2C7CB;' \
'}' \
'QTabBar::tab:!selected {' \
'margin - top: 2px;' \
'}'

ACTIVE_BUTTON_STYLESHEET = "QPushButton { border: 1px solid black; color: #00b4ef; background-color: qlineargradient(" \
                           "spread:pad, x1:1, y1:0, x2:1, y2:1, stop:0  rgba(100, 100, 100, 255), stop:1 rgba(" \
                           "60, 60, 60, 255));} QPushButton:hover {background-color: qlineargradient(x1:0, y1:0, " \
                           "x2:1, y2:1, stop:0 #888888, stop:1 #777777); border: 1px solid #FFFFFF;}"

DISABLE_BUTTON_STYLESHEET = "QPushButton { background-color: qlineargradient(x1:0, y1:0, " \
                           "x2:1, y2:1, stop:0 #888888, stop:1 #777777); border: 1px solid #FFFFFF; color: white;}"


