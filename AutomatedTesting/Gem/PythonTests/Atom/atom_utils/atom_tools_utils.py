"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys
import time
import azlmbr.atom
import azlmbr.atomtools
import azlmbr.bus as bus
import EditorPythonTestTools.editor_python_test_tools.asset_utils as asset_utils

SCREENSHOTS_FOLDER = os.path.join(azlmbr.paths.products, "Screenshots")


def is_close(
        actual: int or float or object, expected: int or float or object, buffer: float = sys.float_info.min) -> bool:
    """
    Obtains the absolute value using an actual value and expected value then compares it to the buffer.
    The result of this comparison is returned as a boolean.
    :param actual: actual value
    :param expected: expected value
    :param buffer: acceptable variation from expected
    :return: bool
    """
    return abs(actual - expected) < buffer


def compare_colors(color1: azlmbr.math.Color, color2: azlmbr.math.Color, buffer: float = 0.00001) -> bool:
    """
    Compares the red, green and blue properties of a color allowing a slight variance of buffer.
    :param color1: azlmbr.math.Color first value to compare.
    :param color2: azlmbr.math.Color second value to compare.
    :param buffer: allowed variance in individual color value.
    :return: boolean representing whether the colors are close (True) or too different (False).
    """
    return (
            is_close(color1.r, color2.r, buffer)
            and is_close(color1.g, color2.g, buffer)
            and is_close(color1.b, color2.b, buffer)
    )


def open_document(file_path: str) -> azlmbr.math.Uuid:
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "OpenDocument", file_path)


def is_document_open(document_id: azlmbr.math.Uuid) -> bool:
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "IsDocumentOpen", document_id)


def save_document(document_id: azlmbr.math.Uuid) -> bool:
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "SaveDocument", document_id)


def save_document_as_copy(document_id: azlmbr.math.Uuid, target_path: str) -> bool:
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(
        bus.Broadcast, "SaveDocumentAsCopy", document_id, target_path)


def save_document_as_child(document_id: azlmbr.math.Uuid, target_path: str) -> bool:
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(
        bus.Broadcast, "SaveDocumentAsChild", document_id, target_path)


def save_all_documents() -> bool:
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "SaveAllDocuments")


def close_document(document_id: azlmbr.math.Uuid):
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "CloseDocument", document_id)


def close_all_documents() -> bool:
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "CloseAllDocuments")


def close_all_except_selected(document_id: azlmbr.math.Uuid) -> bool:
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "CloseAllDocumentsExcept", document_id)


def is_pane_visible(pane_name: str) -> bool:
    return azlmbr.atomtools.AtomToolsMainWindowRequestBus(bus.Broadcast, "IsDockWidgetVisible", pane_name)


def set_pane_visibility(pane_name: str, value: bool) -> None:
    azlmbr.atomtools.AtomToolsMainWindowRequestBus(bus.Broadcast, "SetDockWidgetVisible", pane_name, value)


def select_lighting_config(asset_path: str) -> None:
    asset_id = asset_utils.Asset.find_asset_by_path(asset_path)
    azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(
        azlmbr.bus.Broadcast, "LoadLightingPresetByAssetId", asset_id)


def set_grid_enabled(value: bool) -> None:
    azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "SetGridEnabled", value)


def get_grid_enable_disable() -> bool:
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "GetGridEnabled")


def set_shadowcatcher_enable_disable(value: bool) -> None:
    azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "SetShadowCatcherEnabled", value)


def is_shadowcatcher_enabled() -> bool:
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "GetShadowCatcherEnabled")


def select_model_config(asset_path: str) -> None:
    asset_id = asset_utils.Asset.find_asset_by_path(asset_path)
    azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "LoadModelPresetByAssetId", asset_id)


def undo(document_id: azlmbr.math.Uuid) -> bool:
    return azlmbr.atomtools.AtomToolsDocumentRequestBus(bus.Event, "Undo", document_id)


def redo(document_id: azlmbr.math.Uuid) -> bool:
    return azlmbr.atomtools.AtomToolsDocumentRequestBus(bus.Event, "Redo", document_id)


def begin_edit(document_id: azlmbr.math.Uuid) -> bool:
    return azlmbr.atomtools.AtomToolsDocumentRequestBus(azlmbr.bus.Event, "BeginEdit", document_id)


def end_edit(document_id: azlmbr.math.Uuid) -> bool:
    return azlmbr.atomtools.AtomToolsDocumentRequestBus(azlmbr.bus.Event, "EndEdit", document_id)


def crash() -> None:
    azlmbr.atomtools.general.crash()


def exit() -> None:
    """
    Closes the current Atom tools executable.
    :return: None
    """
    azlmbr.atomtools.general.exit()


def wait_for_condition(function: (), timeout_in_seconds: float = 1.0) -> bool or TypeError:
    """
    Function to run until it returns True or timeout is reached
    the function can have no parameters and
    waiting idle__wait_* is handled here not in the function

    :param function: a function that returns a boolean indicating a desired condition is achieved
    :param timeout_in_seconds: when reached, function execution is abandoned and False is returned
    :return: boolean representing success (True) or failure (False)
    """
    with Timeout(timeout_in_seconds) as t:
        while True:
            try:
                azlmbr.atomtools.general.idle_wait_frames(1)
            except Exception as e:
                print(f"WARNING: Couldn't wait for frame, got Exception : {e}")

            if t.timed_out:
                return False

            ret = function()
            if not isinstance(ret, bool):
                raise TypeError("return value for wait_for_condition function must be a bool")
            if ret:
                return True


class Timeout(object):
    """
    Contextual timeout
    """

    def __init__(self, seconds: float) -> None:
        """
        :param seconds: float seconds to allow before timed_out is True
        :return: None
        """
        self.seconds = seconds

    def __enter__(self) -> object:
        self.die_after = time.time() + self.seconds
        return self

    def __exit__(self, type: any, value: any, traceback: str) -> None:
        pass

    @property
    def timed_out(self) -> float:
        return time.time() > self.die_after


class ScreenshotHelper(object):
    """
    A helper to capture screenshots and wait for them.
    """

    def __init__(self, idle_wait_frames_callback: ()) -> None:
        super().__init__()
        self.done = False
        self.captured_screenshot = False
        self.max_frames_to_wait = 60

        self.idle_wait_frames_callback = idle_wait_frames_callback

    def capture_screenshot_blocking(self, filename: str) -> bool:
        """
        Capture a screenshot and block the execution until the screenshot has been written to the disk.
        """
        self.done = False
        self.captured_screenshot = False
        frame_capture_id = azlmbr.atom.FrameCaptureRequestBus(azlmbr.bus.Broadcast, "CaptureScreenshot", filename)
        if frame_capture_id != -1:
            self.handler = azlmbr.atom.FrameCaptureNotificationBusHandler()
            self.handler.connect(frame_capture_id)
            self.handler.add_callback("OnCaptureFinished", self.on_screenshot_captured)
            self.wait_until_screenshot()
            print("Screenshot taken.")
        else:
            print("screenshot failed")
        return self.captured_screenshot

    def on_screenshot_captured(self, parameters: tuple[any, any]) -> None:
        """
        Sets the value for self.captured_screenshot based on the values in the parameters tuple.
        :param parameters: A tuple of 2 values that indicates if the capture was successful.
        """
        if parameters[0]:
            print(f"screenshot saved: {parameters[1]}")
            self.captured_screenshot = True
        else:
            print(f"screenshot failed: {parameters[1]}")
        self.done = True
        self.handler.disconnect()

    def wait_until_screenshot(self) -> None:
        frames_waited = 0
        while self.done is False:
            self.idle_wait_frames_callback(1)
            if frames_waited > self.max_frames_to_wait:
                print("timeout while waiting for the screenshot to be written")
                self.handler.disconnect()
                break
            else:
                frames_waited = frames_waited + 1
        print(f"(waited {frames_waited} frames)")


def capture_screenshot(file_path: str) -> bool:
    return ScreenshotHelper(azlmbr.atomtools.general.idle_wait_frames).capture_screenshot_blocking(
        os.path.join(file_path)
    )
