"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import logging
import sys

from PySide2.QtCore import Qt
from PySide2.QtWidgets import QApplication

from manager.configuration_manager import ConfigurationManager
from manager.controller_manager import ControllerManager
from manager.thread_manager import ThreadManager
from manager.view_manager import ViewManager
from style import azqtcomponents_resources
from utils import file_utils

# logging setup
logging.basicConfig(filename="resource_mapping_tool.log", filemode='w', level=logging.INFO,
                    format='%(asctime)s,%(msecs)d %(name)s %(levelname)s %(message)s', datefmt='%H:%M:%S')
logging.getLogger('boto3').setLevel(logging.CRITICAL)
logging.getLogger('botocore').setLevel(logging.CRITICAL)
logging.getLogger('s3transfer').setLevel(logging.CRITICAL)
logging.getLogger('urllib3').setLevel(logging.CRITICAL)

logger = logging.getLogger(__name__)
if __name__ == "__main__":
    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling)
    QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps)
    app: QApplication = QApplication(sys.argv)

    try:
        style_sheet_path: str = file_utils.join_path(file_utils.get_parent_directory_path(__file__),
                                                     'style/base_style_sheet.qss')
        with open(style_sheet_path, "r") as in_file:
            style_sheet: str = in_file.read()
            app.setStyleSheet(style_sheet)
    except FileNotFoundError:
        logger.warning("Failed to load style sheet for resource mapping tool")

    logger.info("Initializing configuration manager ...")
    configuration_manager: ConfigurationManager = ConfigurationManager()
    configuration_manager.setup()

    logger.info("Initializing thread manager ...")
    thread_manager: ThreadManager = ThreadManager()
    thread_manager.setup()

    logger.info("Initializing view manager ...")
    view_manager: ViewManager = ViewManager()
    view_manager.setup()

    logger.info("Initializing controller manager ...")
    controller_manager: ControllerManager = ControllerManager()
    controller_manager.setup()
    
    view_manager.show()
    
    sys.exit(app.exec_())
