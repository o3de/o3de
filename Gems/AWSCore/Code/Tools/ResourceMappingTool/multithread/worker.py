"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
