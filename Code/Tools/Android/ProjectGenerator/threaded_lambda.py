#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import threading

class ThreadedLambda:
    """
    A base class that encapsulates a long operation
    via a lambda function that runs on a thread. Typically
    the long operation is a subprocess or a series of subprocesses.
    """
    def __init__(self, basic_description: str, job_func):
        """
        @param job_func The function that will do the work. It should check the state of
                        self._is_cancelled and other member variables and cancel pending work
                        if needed.
        """
        self._basic_description = basic_description
        self._is_finished = False
        self._is_success = False
        # The job_func should check for the status of this variable and exit immediately if needed.
        self._is_cancelled = False
        # Subclasses should store here the result of stdout+stderr.
        self._report_msg = ""
        self._thread = threading.Thread(target=job_func)


    def start(self):
        """
        Do not override.
        """
        self._thread.start()


    def is_finished(self) -> bool:
        return self._is_finished


    def is_success(self) -> bool:
        return self._is_success


    def cancel(self):
        if self._is_cancelled:
            print(f"Job <{self._basic_description}> is already cancelled!")
            return
        self._is_cancelled = True
        self._cancel_internal()
        self._thread.join()


    def get_report_msg(self) -> str:
        return self._report_msg


    def get_basic_description(self) -> str:
        return self._basic_description


    def _cancel_internal(self):
        """
        A protected method that gives the chance to the subclass
        to do something when the operation is being cancelled.
        """
        pass

# class ThreadedLambda END
######################################################
