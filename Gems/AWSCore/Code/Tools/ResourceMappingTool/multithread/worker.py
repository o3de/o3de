"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import sys
import traceback
from PySide2.QtCore import (QObject, QRunnable, Signal, Slot)


class WorkerSignals(QObject):
    """
    Defines the signals available from a running worker thread
    """
    finished: Signal = Signal()
    error: Signal = Signal(tuple)
    result: Signal = Signal(object)


class FunctionWorker(QRunnable):
    """
    Custom worker, which is inheriting from QRunnable to handle worker thread
    setup, signals and wrap-up.
    """
    def __init__(self, function: any, *args: str, **kwargs: int) -> None:
        super(FunctionWorker, self).__init__()
        self.function = function
        self.args = args
        self.kwargs = kwargs
        self.signals: WorkerSignals = WorkerSignals()

    @Slot()
    def run(self) -> None:
        try:
            result: object = self.function(*self.args, **self.kwargs)
        except:  # catch all exceptions for this generic worker
            traceback.print_exc()
            exctype, value = sys.exc_info()[:2]
            self.signals.error.emit((exctype, value, traceback.format_exc()))
        else:
            self.signals.result.emit(result)
        finally:
            self.signals.finished.emit()
