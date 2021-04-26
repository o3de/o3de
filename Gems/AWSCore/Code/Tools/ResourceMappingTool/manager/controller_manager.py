"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from __future__ import annotations
import logging

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
            self._view_edit_controller: ViewEditController = ViewEditController()
            self._import_resources_controller: ImportResourcesController = ImportResourcesController()
            ControllerManager.__instance = self
        else:
            raise AssertionError(error_messages.SINGLETON_OBJECT_ERROR_MESSAGE.format("ControllerManager"))
    
    @property
    def import_resources_controller(self) -> ImportResourcesController:
        return self._import_resources_controller

    @property
    def view_edit_controller(self) -> ViewEditController:
        return self._view_edit_controller
    
    def setup(self) -> None:
        logger.info("Setting up ViewEdit and ImportResource controllers ...")
        self._view_edit_controller.setup()
        self._import_resources_controller.setup()
        self._import_resources_controller.add_import_resources_sender.connect(
            self._view_edit_controller.add_import_resources_receiver)
