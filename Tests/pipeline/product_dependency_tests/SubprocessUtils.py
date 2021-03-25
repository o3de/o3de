"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Automated scripts for tests calling AssetProcessorBatch validating basic features.

"""

import subprocess
import threading
import time

class ThreadedSubprocess():
    def __init__(self, command, workingDirectory, timeOutMinutes):
        self.command = command
        self.workingDirectory = workingDirectory
        self.timeOutSeconds=timeOutMinutes*60
        self.process = None
        self.logOutput = []
        # Pytest doesn't handle asserts on other threads, capture them and report on the main thread.
        self.assertError = None

    def RunCommand(self):
        def RunThread():
            print (str.format("Subprocess thread starting for command: {}", self.command))
            self.process = subprocess.Popen(self.command, cwd=self.workingDirectory, shell=True, stdout=subprocess.PIPE, universal_newlines=True)
            for stdoutLine in iter(self.process.stdout.readline, ""):
                self.logOutput.append(stdoutLine)
            self.process.communicate()
            if self.process.returncode is None:
                self.assertError = str.format("Subprocess call '{}' had no return code", self.command)
            elif self.process.returncode != 0:
                self.assertError = str.format("Subprocess call '{}' returned code {}", self.command, self.process.returncode)
            print (str.format("Finished command, result {}: {}", self.process.returncode, self.command))

        commandThread = threading.Thread(target=RunThread)
        commandThread.start()
        commandThread.join(self.timeOutSeconds)
        assert not commandThread.is_alive(), str.format("Subprocess call '{}' timed out", self.command)
        assert self.assertError is None, self.assertError


def SubprocessWithTimeout(command, workingDirectory, timeOutMinutes):
    threadedSubprocess = ThreadedSubprocess(command, workingDirectory, timeOutMinutes)
    threadedSubprocess.RunCommand()
    return threadedSubprocess.logOutput
