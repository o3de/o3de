"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations
import logging

from controller.error_controller import ErrorController
from controller.import_resources_controller import ImportResourcesController
from controller.view_edit_controller import ViewEditController
from model import error_messages

logger = logging.getLogger(__name__)


class ControllerManager(object):
    """
    Controller manager maintains all view controllers for this tool
    """
    __instance: ControllerManager = None
    
    @staticmethod
    def get_instance() -> ControllerManager:
        if ControllerManager.__instance is None:
            ControllerManager()
        return ControllerManager.__instance
    
    def __init__(self) -> None:
        if ControllerManager.__instance is None:
            self._error_controller: ErrorController = None
            self._view_edit_controller: ViewEditController = None
            self._import_resources_controller: ImportResourcesController = None
            ControllerManager.__instance = self
        else:
            raise AssertionError(error_messages.SINGLETON_OBJECT_ERROR_MESSAGE.format("ControllerManager"))
    
    @property
    def import_resources_controller(self) -> ImportResourcesController:
        return self._import_resources_controller

    @property
    def view_edit_controller(self) -> ViewEditController:
        return self._view_edit_controller
    
    def setup(self, setup_error: bool) -> None:
        if setup_error:
            logger.debug("Setting up Error controllers ...")
            self._error_controller = ErrorController()
            self._error_controller.setup()
        else:
            logger.debug("Setting up ViewEdit and ImportResource controllers ...")
            self._view_edit_controller = ViewEditController()
            self._import_resources_controller = ImportResourcesController()

            self._view_edit_controller.setup()
            self._import_resources_controller.setup()
            self._import_resources_controller.add_import_resources_sender.connect(
                self._view_edit_controller.add_import_resources_receiver)
