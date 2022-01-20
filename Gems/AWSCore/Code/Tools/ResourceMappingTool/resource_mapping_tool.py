"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from argparse import (ArgumentParser, Namespace)
import logging
import sys

from utils import environment_utils
from utils import json_utils
from utils import file_utils

# arguments setup
argument_parser: ArgumentParser = ArgumentParser()
argument_parser.add_argument('--binaries-path', help='Path to QT Binaries necessary for PySide.')
argument_parser.add_argument('--config-path', help='Path to resource mapping config directory.')
argument_parser.add_argument('--debug', action='store_true', help='Execute on debug mode to enable DEBUG logging level')
argument_parser.add_argument('--log-path', help='Path to resource mapping tool logging directory '
                                                '(if not provided, logging file will be located at tool directory)')
argument_parser.add_argument('--profile', default='default', help='Named AWS profile to use for querying AWS resources')

arguments: Namespace = argument_parser.parse_args()

# logging setup
logging_level: int = logging.INFO
if arguments.debug:
    logging_level = logging.DEBUG
logging_path: str = file_utils.join_path(file_utils.get_parent_directory_path(__file__), 'resource_mapping_tool.log')
if arguments.log_path:
    normalized_logging_path: str = file_utils.normalize_file_path(arguments.log_path, False)
    if normalized_logging_path and file_utils.create_directory(normalized_logging_path):
        logging_path = file_utils.join_path(normalized_logging_path, 'resource_mapping_tool.log')
logging.basicConfig(filename=logging_path, filemode='w', level=logging_level,
                    format='%(asctime)s,%(msecs)d %(name)s %(levelname)s %(message)s', datefmt='%H:%M:%S')
logging.getLogger('boto3').setLevel(logging.CRITICAL)
logging.getLogger('botocore').setLevel(logging.CRITICAL)
logging.getLogger('s3transfer').setLevel(logging.CRITICAL)
logging.getLogger('urllib3').setLevel(logging.CRITICAL)
logger = logging.getLogger(__name__)
logger.info(f"Using {logging_path} for logging.")

if __name__ == "__main__":
    if arguments.binaries_path and not environment_utils.is_qt_linked():
        logger.info("Setting up Qt environment ...")
        environment_utils.setup_qt_environment(arguments.binaries_path)

    try:
        logger.info("Importing tool required modules ...")
        from PySide2.QtCore import Qt
        from PySide2.QtWidgets import QApplication
        from manager.configuration_manager import ConfigurationManager
        from manager.controller_manager import ControllerManager
        from manager.thread_manager import ThreadManager
        from manager.view_manager import ViewManager
        from style import azqtcomponents_resources
        from style import editormainwindow_resources
    except ImportError as e:
        logger.error(f"Failed to import module [{e.name}] {e}")
        environment_utils.cleanup_qt_environment()
        exit(-1)

    QApplication.setAttribute(Qt.AA_EnableHighDpiScaling)
    QApplication.setAttribute(Qt.AA_UseHighDpiPixmaps)
    app: QApplication = QApplication(sys.argv)
    app.aboutToQuit.connect(environment_utils.cleanup_qt_environment)

    try:
        style_sheet_path: str = file_utils.join_path(file_utils.get_parent_directory_path(__file__),
                                                     'style/base_style_sheet.qss')
        with open(style_sheet_path, "r") as in_file:
            style_sheet: str = in_file.read()
            app.setStyleSheet(style_sheet)
    except FileNotFoundError:
        logger.warning("Failed to load style sheet for resource mapping tool")

    try:
        schema_path: str = file_utils.join_path(file_utils.get_parent_directory_path(__file__),
                                                'resource_mapping_schema.json')
        json_utils.load_resource_mapping_json_schema(schema_path)
    except (FileNotFoundError, ValueError, KeyError) as e:
        logger.error(f"Failed to load schema file {e}")
        environment_utils.cleanup_qt_environment()
        exit(-1)

    logger.info("Initializing configuration manager ...")
    configuration_manager: ConfigurationManager = ConfigurationManager()
    configuration_error: bool = not configuration_manager.setup(arguments.profile, arguments.config_path)

    logger.info("Initializing thread manager ...")
    thread_manager: ThreadManager = ThreadManager()
    thread_manager.setup(configuration_error)

    logger.info("Initializing view manager ...")
    view_manager: ViewManager = ViewManager()
    view_manager.setup(configuration_error)

    logger.info("Initializing controller manager ...")
    controller_manager: ControllerManager = ControllerManager()
    controller_manager.setup(configuration_error)
    
    view_manager.show(configuration_error)

    sys.exit(app.exec_())
