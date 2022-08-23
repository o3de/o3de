from PySide2 import QtWidgets, QtCore, QtGui


def get_title_bar_widget(target_layout, title, width=None):
    target_layout.setContentsMargins(10, 0, 5, 0)
    title_bar_widget = QtWidgets.QWidget()
    title_bar_widget.setLayout(target_layout)
    title_bar_frame = QtWidgets.QFrame(title_bar_widget)
    title_bar_frame.setStyleSheet('background-color: rgb(50, 50, 50);')
    title_bar_frame.setGeometry(0, 0, 5000, 30)
    title_bar_label = QtWidgets.QLabel(title)
    title_bar_label.setFont(QtGui.QFont("Helvetica", 7, QtGui.QFont.Bold))
    target_layout.addWidget(title_bar_label)
    if width:
        title_bar_widget.setFixedSize(width, 30)
    else:
        title_bar_widget.setFixedHeight(30)

    return title_bar_widget

