"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations
import logging
from PySide2.QtCore import (QRunnable, QThreadPool)

from model import error_messages

logger = logging.getLogger(__name__)


class ThreadManager(object):
    """
    Thread manager maintains thread pool for executing/tracking thread worker
    """
    __instance: ThreadManager = None

    @staticmethod
    def get_instance() -> ThreadManager:
        if ThreadManager.__instance is None:
            ThreadManager()
        return ThreadManager.__instance

    def __init__(self) -> None:
        if ThreadManager.__instance is None:
            self._thread_pool: QThreadPool = QThreadPool()
            ThreadManager.__instance = self
        else:
            raise AssertionError(error_messages.SINGLETON_OBJECT_ERROR_MESSAGE.format("ThreadManager"))

    def setup(self, setup_error: bool, thread_count: int = 1) -> None:
        if setup_error:
            logger.debug("Skip thread pool creation, as there is major setup error.")
        else:
            # Based on prototype use case, we just need 1 thread
            logger.debug(f"Setting up thread pool with MaxThreadCount={thread_count} ...")
            self._thread_pool.setMaxThreadCount(thread_count)

    """Reserves a thread and uses it to run runnable worker, unless this thread will make
    the current thread count exceed max thread count. In that case, runnable is added to a run queue instead."""
    def start(self, worker: QRunnable) -> None:
        self._thread_pool.start(worker)

    """Removes the runnables that are not yet started from the queue. """
    def clear(self) -> None:
        self._thread_pool.clear()
