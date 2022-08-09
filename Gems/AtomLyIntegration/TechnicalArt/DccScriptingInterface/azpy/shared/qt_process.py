import config
from dynaconf import settings
from pathlib import Path
from PySide2 import QtCore
from PySide2.QtCore import Signal, Slot, QProcess
import logging
import json


_LOGGER = logging.getLogger('azpy.shared.qt_process')
_LOGGER.info('Start QT Process...')


class QtProcess(QtCore.QObject):
    process_info = Signal(dict)

    def __init__(self, application, target_file, **kwargs):
        super(QtProcess, self).__init__()

        _LOGGER.info('QProcess added...')
        self.application = application
        self.target_file = target_file
        self.environment_variables = self.get_environment_variables(kwargs)
        self.p = None
        self.process_output = {}
        self.initialize_qprocess()

    def initialize_qprocess(self):
        """
        This sets the QProcess object up and adds all of the aspects that will remain consistent for each directory
        processed.
        :return:
        """
        _LOGGER.info(f'Application being passed: {self.application}')
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
        self.p.finished.connect(self.cleanup)

    def get_environment_variables(self, kwargs):
        env = [env for env in QtCore.QProcess.systemEnvironment() if not env.startswith('PYTHONPATH=')]
        for key, value in kwargs.items():
            env.append(f'{key}={value}')
        return env

    def process_started(self):
        _LOGGER.info('QProcess Started....')

    def handle_stderr(self):
        data = str(self.p.readAllStandardError(), 'utf-8')
        _LOGGER.info(f'STD_ERROR_FIRED: {data}')

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

    def process_complete(self):
        _LOGGER.info('Process Completed.')

    def cleanup(self):
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
        _LOGGER.info(f'Process started')
        try:
            if detached:
                self.p.startDetached()
            else:
                self.p.start()
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

