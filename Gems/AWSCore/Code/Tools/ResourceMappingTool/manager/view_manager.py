"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations
import logging

from PySide2.QtGui import QIcon
from PySide2.QtWidgets import (QMainWindow, QStackedWidget, QWidget)

from model import (error_messages, view_size_constants)
from view.error_page import ErrorPage
from view.import_resources_page import ImportResourcesPage
from view.view_edit_page import ViewEditPage

logger = logging.getLogger(__name__)


class ViewManagerConstants(object):
    VIEW_AND_EDIT_PAGE_INDEX: int = 0
    IMPORT_RESOURCES_PAGE_INDEX: int = 1


# Error page will be a single page
ERROR_PAGE_INDEX: int = 0


class ViewManager(object):
    """
    View manager maintains the main stacked pages for this tool, which
    includes ViewEdit, ImportResources and ImportStacks these three views
    """
    __instance: ViewManager = None
    
    @staticmethod
    def get_instance() -> ViewManager:
        if ViewManager.__instance is None:
            ViewManager()
        return ViewManager.__instance

    def __init__(self) -> None:
        if ViewManager.__instance is None:
            self._main_window: QMainWindow = QMainWindow()
            self._main_window.setWindowIcon(QIcon(":/Application/o3de_application_reverse.svg"))
            self._main_window.setWindowTitle("Resource Mapping")
            self._main_window.setGeometry(0, 0,
                                          view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                          view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_HEIGHT)
            self._main_window.setMaximumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                             view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_HEIGHT)

            self._resource_mapping_stacked_pages: QStackedWidget = QStackedWidget()
            self._resource_mapping_stacked_pages.setObjectName("ResourceMappingStackedPages")

            self._main_window.setCentralWidget(self._resource_mapping_stacked_pages)
            ViewManager.__instance = self
        else:
            raise AssertionError(error_messages.SINGLETON_OBJECT_ERROR_MESSAGE.format("ViewManager"))

    def get_error_page(self) -> QWidget:
        return self._resource_mapping_stacked_pages.widget(ERROR_PAGE_INDEX)

    def get_view_edit_page(self) -> QWidget:
        return self._resource_mapping_stacked_pages.widget(ViewManagerConstants.VIEW_AND_EDIT_PAGE_INDEX)

    def get_import_resources_page(self) -> QWidget:
        return self._resource_mapping_stacked_pages.widget(ViewManagerConstants.IMPORT_RESOURCES_PAGE_INDEX)
    
    def setup(self, setup_error: bool) -> None:
        if setup_error:
            logger.debug("Setting up Error view pages ...")
            self._resource_mapping_stacked_pages.addWidget(ErrorPage())
            self._main_window.adjustSize()  # fit error page size
        else:
            logger.debug("Setting up ViewEdit and ImportResources view pages ...")
            self._resource_mapping_stacked_pages.addWidget(ViewEditPage())
            self._resource_mapping_stacked_pages.addWidget(ImportResourcesPage())

    def show(self, setup_error: bool) -> None:
        """Show up the tool view by setting default page index and showing main widget"""
        if setup_error:
            self._resource_mapping_stacked_pages.setCurrentIndex(ERROR_PAGE_INDEX)
        else:
            self._resource_mapping_stacked_pages.setCurrentIndex(ViewManagerConstants.VIEW_AND_EDIT_PAGE_INDEX)
        self._main_window.show()

    def switch_to_view_edit_page(self) -> None:
        """Switch tool view to ViewEdit page"""
        self._resource_mapping_stacked_pages.setCurrentIndex(ViewManagerConstants.VIEW_AND_EDIT_PAGE_INDEX)

    def switch_to_import_resources_page(self, search_version: str) -> None:
        """Switch tool view to ImportResources page"""
        import_resources_page: ImportResourcesPage = \
            self._resource_mapping_stacked_pages.widget(ViewManagerConstants.IMPORT_RESOURCES_PAGE_INDEX)
        import_resources_page.search_version = search_version
        self._resource_mapping_stacked_pages.setCurrentIndex(ViewManagerConstants.IMPORT_RESOURCES_PAGE_INDEX)
