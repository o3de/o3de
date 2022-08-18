# import config
# from dynaconf import settings
# from pathlib import Path
from PySide2 import QtCore
from PySide2.QtCore import Signal, Slot, QProcess
from azpy import constants
import logging
import json


_LOGGER = logging.getLogger('azpy.shared.qt_process')
_LOGGER.info('Starting QT Process...')


class QtProcess(QtCore.QObject):
    process_info = Signal(dict)

    def __init__(self, application, target_files, data):
        super(QtProcess, self).__init__()

        self.p = None
        self.application = application
        self.target_files = target_files
        self.data = data
        self.environment_variables = self.parse_data()
        self.processing_script = self.parse_data('SCRIPT')
        self.input_data = self.parse_data('INPUT_DATA')
        self.output_data = self.parse_data('OUTPUT_DATA')
        self.process_output = {}
        self.initialize_qprocess()

    def initialize_qprocess(self):
        """
        This sets the QProcess object up and adds all of the aspects that will remain consistent for each directory
        processed.
        :return:
        """
        _LOGGER.info(f'\n{constants.STR_CROSSBAR}\n::QPROCESS LAUNCHED::::> Application: {self.application}')
        _LOGGER.info(f'Target File: {self.target_files}')
        _LOGGER.info(f'Processing Script: {self.processing_script}')
        _LOGGER.info(f'Output Information: {self.output_data}\n{constants.STR_CROSSBAR}')

        self.p = QProcess()
        env = QtCore.QProcessEnvironment.systemEnvironment()
        for item in self.environment_variables:
            values = item.split('=')
            env.insert(values[0], values[1])
        self.p.setProcessEnvironment(env)
        self.p.setProgram(self.application)
        self.p.setProcessChannelMode(QProcess.SeparateChannels)
        self.p.readyReadStandardOutput.connect(self.handle_stdout)
        self.p.readyReadStandardError.connect(self.handle_stderr)
        self.p.stateChanged.connect(self.handle_state)
        self.p.started.connect(self.process_started)
        self.p.finished.connect(self.process_complete)

    def parse_data(self, target_key=None):
        """
        The kwargs argument when instantiating the QProcess class is mainly used for establishing the application
        environment, with the exception of passing execution scripts as well as also sending output information when
        the application process is launched. The method for achieving this is to insert "script" and/or
        "input_data/output_data" as keys, and related information as values
        """
        env = [env for env in QtCore.QProcess.systemEnvironment() if not env.startswith('PYTHONPATH=')]
        for element in self.data:
            listing = element.split('=')
            key = listing[0]
            value = '='.join(listing[1:])
            if key == target_key:
                return value
            else:
                env.append(f'{key}={value}')
        if target_key:
            return None
        return env

    def handle_stderr(self):
        pass
        # Keep this enabled unless you need to register all events while debugging
        # data = str(self.p.readAllStandardError(), 'utf-8')
        # _LOGGER.info(f'STD_ERROR_FIRED: {data}')

    def handle_stdout(self):
        """
        This catches standard output from Maya while processing. It is used for keeping track of progress, and once
        the last FBX file in a target directory is processed, it updates the database with the newly created Maya files.
        :return:
        """
        data = str(self.p.readAllStandardOutput(), 'utf-8')
        _LOGGER.info(f'STDOUT Firing: {data}')
        if data.startswith('{'):
            self.process_output = json.loads(data)
            self.process_info.emit(self.process_output)
            _LOGGER.info('QProcess Complete.')

    def handle_state(self, state):
        """
        Indicates in logging when a process becomes active, and where it stands until completion
        :param state:
        :return:
        """
        states = {
            QProcess.NotRunning: 'Not running',
            QProcess.Starting:   'Starting',
            QProcess.Running:    'Running'
        }
        state_name = states[state]
        _LOGGER.info(f'QProcess State Change: {state_name}')

    def process_started(self):
        _LOGGER.info('QProcess Started....')

    def process_complete(self):
        _LOGGER.info('QProcess Completed.')
        self.p.closeWriteChannel()

    def start_process(self, detached=True):
        """
        This starts the exchange between the standalone QT UI and Maya Standalone to process FBX files.
        The information sent to Maya is the FBX file for processing, as well as the base directory and
        relative destination paths. There is a pipe that can be used to shuttle processed information
        back to the script, but at this stage it is not being used.
        :param detached: Set detached to False if the operation you are performing needs to be ran, and
        the process subsequently closed.
        :return:
        """

        info_list = [self.target_files, self.input_data, self.output_data]
        command = [self.processing_script]
        for entry in info_list:
            command.append(str(entry))
        self.p.setArguments(command)
        _LOGGER.info(f'\n\nCommand:::::>\n{command}')
        _LOGGER.info(self.p.arguments())

        try:
            if detached:
                self.p.startDetached()
            else:
                self.p.start()
                self.p.waitForFinished(-1)
        except Exception as e:
            _LOGGER.info(f'QProcess failed: {e}')

    @Slot(dict)
    def get_process_output(self):
        _LOGGER.info(f'ProcessOutputFired: {self.process_output}')
        return self.process_output


if __name__ == '__main__':
    pass
    # Use this area to test output:
    # maya_path = r"C:\Program Files\Autodesk\Maya2023\bin\maya.exe"
    # maya = QtProcess(maya_path, '')
    # maya.start_process()
    #
    # from azpy.dcc.maya.utils import maya_server
    # maya_server.launch()

