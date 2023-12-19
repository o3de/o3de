#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import subprocess

class SubprocessRunner:
    """
    Class for common methods and properties
    required to dispatch a shell/system/python application
    and capture the errorCode, stdout and stderr
    """

    def __init__(self, argList: list[str], timeOutSeconds: int, cwd: str = ".") :
        self._name = self.__class__.__name__
        self._argList = argList
        self._arg_list_str = ""  # Becomes the command string when executed.
        self._timeOut = timeOutSeconds
        self._error_code = -1
        self._error_message = ""
        self._success_message = ""
        self._cwd = cwd


    def run(self) -> bool:
        """
        This is a blocking call that returns when the subprocess is finished.
        @returns True if the system error code is 0 (success).
        """
        self._arg_list_str = " ".join(self._argList)
        print(f"{self._name} will run command:\n{self._arg_list_str}\n")
        self._subprocess = subprocess.Popen(
            self._argList, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=self._cwd
        )
        try:
            outs, errs = self._subprocess.communicate(timeout=self._timeOut)
            self._success_message = outs.decode("utf-8")
            self._error_message = errs.decode("utf-8")
            print(f"ok:<{self._success_message}>, err:<{self._error_message}>")
            self._error_code = self._subprocess.returncode
            return self._subprocess.returncode == 0
        except subprocess.TimeoutExpired:
            self._subprocess.kill()
            outs, errs = self._subprocess.communicate()
            self._success_message = outs.decode("utf-8")
            self._error_message = errs.decode("utf-8")
            print(f"ok:<{self._success_message}>, err:<{self._error_message}>")
            self._error_code = -1
            return False


    def get_error_code(self):
        return self._error_code


    def get_stderr(self) -> str:
        return self._error_message


    def get_stdout(self) -> str:
        return self._success_message


    def get_stdall(self) -> str:
        return f"{self._success_message}\n{self._error_message}"


    def get_command_str(self) -> str:
        return self._arg_list_str

# class SubprocessRunner END
######################################################
