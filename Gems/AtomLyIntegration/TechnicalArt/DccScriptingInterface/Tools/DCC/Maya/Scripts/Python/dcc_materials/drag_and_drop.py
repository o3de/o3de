"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

from PySide2 import QtWidgets, QtCore
from PySide2.QtCore import Signal


class DragAndDrop(QtWidgets.QWidget):
    drop_update = QtCore.Signal(list)
    drop_over = QtCore.Signal(bool)

    def __init__(self, frame_color=None, highlight=None, parent=None):
        super(DragAndDrop, self).__init__(parent)

        self.urls = []
        self.frame_color = frame_color
        self.frame_highlight = highlight
        self.setContentsMargins(0, 0, 0, 0)
        self.setAcceptDrops(True)

        self.drag_and_drop_frame = QtWidgets.QFrame(self)
        self.drag_and_drop_frame.setGeometry(0, 0, 5000, 5000)
        self.drag_and_drop_frame.setStyleSheet('background-color:rgb({});'.format(self.frame_color))

    def dragEnterEvent(self, e):
        if e.mimeData().hasUrls:
            e.accept()
            self.drop_over.emit(True)
            if self.frame_highlight:
                self.drag_and_drop_frame.setStyleSheet('background-color:rgb({});'.format(self.frame_highlight))
        else:
            e.ignore()

    def dragLeaveEvent(self, e):
        self.drop_over.emit(False)

        if self.frame_highlight:
            self.drag_and_drop_frame.setStyleSheet('background-color:rgb({});'.format(self.frame_color))


    def dragMoveEvent(self, e):
        if e.mimeData().hasUrls:
            e.accept()
        else:
            e.ignore()

    def dropEvent(self, e):
        if e.mimeData().hasUrls:
            e.setDropAction(QtCore.Qt.CopyAction)
            e.accept()

            for url in e.mimeData().urls():
                file_name = str(url.toLocalFile())
                self.urls.append(file_name)
            self.drop_update.emit(self.urls)
            if self.frame_highlight:
                self.drag_and_drop_frame.setStyleSheet('background-color:rgb({});'.format(self.frame_color))
        else:
            e.ignore()
