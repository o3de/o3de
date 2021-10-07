"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

import azlmbr.materialeditor will fail with a ModuleNotFound error when using this script with Editor.exe
This is because azlmbr.materialeditor only binds to MaterialEditor.exe and not Editor.exe
You need to launch this script with MaterialEditor.exe in order for azlmbr.materialeditor to appear.
"""

import os
import sys
import time
import azlmbr.atom
import azlmbr.atomtools as atomtools
import azlmbr.materialeditor as materialeditor
import azlmbr.bus as bus


def is_close(actual, expected, buffer=sys.float_info.min):
    """
    :param actual: actual value
    :param expected: expected value
    :param buffer: acceptable variation from expected
    :return: bool
    """
    return abs(actual - expected) < buffer


def compare_colors(color1, color2, buffer=0.00001):
    """
    Compares the red, green and blue properties of a color allowing a slight variance of buffer
    :param color1: first color to compare
    :param color2: second color
    :param buffer: allowed variance in individual color value
    :return: bool
    """
    return (
            is_close(color1.r, color2.r, buffer)
            and is_close(color1.g, color2.g, buffer)
            and is_close(color1.b, color2.b, buffer)
    )


def open_material(file_path):
    """
    :return: uuid of material document opened
    """
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "OpenDocument", file_path)


def is_open(document_id):
    """
    :return: bool
    """
    return azlmbr.atomtools.AtomToolsDocumentRequestBus(bus.Event, "IsOpen", document_id)


def save_document(document_id):
    """
    :return: bool success
    """
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "SaveDocument", document_id)


def save_document_as_copy(document_id, target_path):
    """
    :return: bool success
    """
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(
        bus.Broadcast, "SaveDocumentAsCopy", document_id, target_path
    )


def save_document_as_child(document_id, target_path):
    """
    :return: bool success
    """
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(
        bus.Broadcast, "SaveDocumentAsChild", document_id, target_path
    )


def save_all():
    """
    :return: bool success
    """
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "SaveAllDocuments")


def close_document(document_id):
    """
    :return: bool success
    """
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "CloseDocument", document_id)


def close_all_documents():
    """
    :return: bool success
    """
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "CloseAllDocuments")


def close_all_except_selected(document_id):
    """
    :return: bool success
    """
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(bus.Broadcast, "CloseAllDocumentsExcept", document_id)


def get_property(document_id, property_name):
    """
    :return: property value or invalid value if the document is not open or the property_name can't be found
    """
    return azlmbr.atomtools.AtomToolsDocumentRequestBus(bus.Event, "GetPropertyValue", document_id, property_name)


def set_property(document_id, property_name, value):
    azlmbr.atomtools.AtomToolsDocumentRequestBus(bus.Event, "SetPropertyValue", document_id, property_name, value)


def is_pane_visible(pane_name):
    """
    :return: bool
    """
    return atomtools.AtomToolsWindowRequestBus(bus.Broadcast, "IsDockWidgetVisible", pane_name)


def set_pane_visibility(pane_name, value):
    atomtools.AtomToolsWindowRequestBus(bus.Broadcast, "SetDockWidgetVisible", pane_name, value)


def select_lighting_config(config_name):
    azlmbr.materialeditor.MaterialViewportRequestBus(azlmbr.bus.Broadcast, "SelectLightingPresetByName", config_name)


def set_grid_enable_disable(value):
    azlmbr.materialeditor.MaterialViewportRequestBus(azlmbr.bus.Broadcast, "SetGridEnabled", value)


def get_grid_enable_disable():
    """
    :return: bool
    """
    return azlmbr.materialeditor.MaterialViewportRequestBus(azlmbr.bus.Broadcast, "GetGridEnabled")


def set_shadowcatcher_enable_disable(value):
    azlmbr.materialeditor.MaterialViewportRequestBus(azlmbr.bus.Broadcast, "SetShadowCatcherEnabled", value)


def get_shadowcatcher_enable_disable():
    """
    :return: bool
    """
    return azlmbr.materialeditor.MaterialViewportRequestBus(azlmbr.bus.Broadcast, "GetShadowCatcherEnabled")


def select_model_config(configname):
    azlmbr.materialeditor.MaterialViewportRequestBus(azlmbr.bus.Broadcast, "SelectModelPresetByName", configname)


def wait_for_condition(function, timeout_in_seconds=1.0):
    # type: (function, float) -> bool
    """
    Function to run until it returns True or timeout is reached
    the function can have no parameters and
    waiting idle__wait_* is handled here not in the function

    :param function: a function that returns a boolean indicating a desired condition is achieved
    :param timeout_in_seconds: when reached, function execution is abandoned and False is returned
    """
    with Timeout(timeout_in_seconds) as t:
        while True:
            try:
                azlmbr.atomtools.general.idle_wait_frames(1)
            except Exception:
                print("WARNING: Couldn't wait for frame")

            if t.timed_out:
                return False

            ret = function()
            if not isinstance(ret, bool):
                raise TypeError("return value for wait_for_condition function must be a bool")
            if ret:
                return True


class Timeout:
    # type: (float) -> None
    """
    contextual timeout
    :param seconds: float seconds to allow before timed_out is True
    """

    def __init__(self, seconds):
        self.seconds = seconds

    def __enter__(self):
        self.die_after = time.time() + self.seconds
        return self

    def __exit__(self, type, value, traceback):
        pass

    @property
    def timed_out(self):
        return time.time() > self.die_after


screenshotsFolder = os.path.join(azlmbr.paths.products, "Screenshots")


class ScreenshotHelper:
    """
    A helper to capture screenshots and wait for them.
    """

    def __init__(self, idle_wait_frames_callback):
        super().__init__()
        self.done = False
        self.capturedScreenshot = False
        self.max_frames_to_wait = 60

        self.idle_wait_frames_callback = idle_wait_frames_callback

    def capture_screenshot_blocking(self, filename):
        """
        Capture a screenshot and block the execution until the screenshot has been written to the disk.
        """
        self.handler = azlmbr.atom.FrameCaptureNotificationBusHandler()
        self.handler.connect()
        self.handler.add_callback("OnCaptureFinished", self.on_screenshot_captured)

        self.done = False
        self.capturedScreenshot = False
        success = azlmbr.atom.FrameCaptureRequestBus(azlmbr.bus.Broadcast, "CaptureScreenshot", filename)
        if success:
            self.wait_until_screenshot()
            print("Screenshot taken.")
        else:
            print("screenshot failed")
        return self.capturedScreenshot

    def on_screenshot_captured(self, parameters):
        # the parameters come in as a tuple
        if parameters[0]:
            print("screenshot saved: {}".format(parameters[1]))
            self.capturedScreenshot = True
        else:
            print("screenshot failed: {}".format(parameters[1]))
        self.done = True
        self.handler.disconnect()

    def wait_until_screenshot(self):
        frames_waited = 0
        while self.done == False:
            self.idle_wait_frames_callback(1)
            if frames_waited > self.max_frames_to_wait:
                print("timeout while waiting for the screenshot to be written")
                self.handler.disconnect()
                break
            else:
                frames_waited = frames_waited + 1
        print("(waited {} frames)".format(frames_waited))


def capture_screenshot(file_path):
    return ScreenshotHelper(azlmbr.atomtools.general.idle_wait_frames).capture_screenshot_blocking(
        os.path.join(file_path)
    )
