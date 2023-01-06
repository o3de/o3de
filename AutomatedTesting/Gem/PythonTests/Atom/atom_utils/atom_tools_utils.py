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
import azlmbr.paths

MATERIAL_TYPES_PATH = os.path.join(
    azlmbr.paths.engroot, "Gems", "Atom", "Feature", "Common", "Assets", "Materials", "Types")
MATERIALCANVAS_GRAPH_PATH = os.path.join(
    azlmbr.paths.engroot, "Gems", "Atom", "Tools", "MaterialCanvas", "Assets", "MaterialCanvas", "TestData")
SCREENSHOTS_FOLDER = os.path.join(azlmbr.paths.products, "Screenshots")
TEST_DATA_MATERIALS_PATH = os.path.join(azlmbr.paths.engroot, "Gems", "Atom", "TestData", "TestData", "Materials")
VIEWPORT_LIGHTING_PRESETS_PATH = os.path.join(
    azlmbr.paths.engroot, "Gems", "Atom", "Tools", "MaterialEditor", "Assets", "MaterialEditor", "LightingPresets")
VIEWPORT_MODELS_PRESETS_PATH = os.path.join(
    azlmbr.paths.engroot, "Gems", "Atom", "Tools", "MaterialEditor", "Assets", "MaterialEditor", "ViewportModels")


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


def verify_one_material_document_opened(
        material_document_ids_list: [azlmbr.math.Uuid], opened_document_index: int) -> bool:
    """
    Validation helper to verify if the document at opened_document_index value in the material_document_ids list
    is the only opened document.
    Returns True on success, False on failure.

    :param material_document_ids_list: List of material document IDs used for making the document opened check.
    :param opened_document_index: Index number of the one material document that should be open
    :return: bool
    """
    if not is_document_open(material_document_ids_list[opened_document_index]):
        return False

    material_document_ids_verification_list = material_document_ids_list.copy()
    material_document_ids_verification_list.pop(opened_document_index)
    for closed_material_document_id in material_document_ids_verification_list:
        if is_document_open(closed_material_document_id):
            return False

    return True


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


def load_lighting_preset_by_asset_id(asset_id: azlmbr.math.Uuid) -> bool:
    """
    Takes in an asset ID and attempts to select the lighting background in the viewporet dropdown using that ID.
    Returns True if it successfully changes it, False otherwise.
    """
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(
        azlmbr.bus.Broadcast, "LoadLightingPresetByAssetId", asset_id)


def load_lighting_preset_by_path(asset_path: str) -> bool:
    """
    Takes in an asset path and attempts to select the lighting background in the viewport dropdown using that path.
    Returns True if it successfully changes it, False otherwise.
    """
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(
        azlmbr.bus.Broadcast, "LoadLightingPreset", asset_path)


def get_last_lighting_preset_asset_id() -> azlmbr.math.Uuid:
    """
    Returns the asset ID of the currently selected viewport lighting background.
    Example return values observed when selecting different lighting backgrounds from the viewport dropdown:
    {B9AA460C-622D-5F38-A02C-C0BCB0B65D96}:0
    {C4978B46-D509-5B71-86A1-09D8CC051DC4}:0
    {AB7FA1BA-7207-5333-BDD6-69C3F5B7A410}:0
    """
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(
        azlmbr.bus.Broadcast, "GetLastLightingPresetAssetId")


def get_last_lighting_preset_path() -> str:
    """
    Returns the full path to the currently selected viewport lighting background.
    Example return values observed when selecting different lighting backgrounds from the viewport dropdown:
    "C:/git/o3de/Gems/Atom/Tools/MaterialEditor/Assets/MaterialEditor/LightingPresets/neutral_urban.lightingpreset.azasset"
    "C:/git/o3de/Gems/Atom/Feature/Common/Assets/LightingPresets/LowContrast/artist_workshop.lightingpreset.azasset"
    "C:/git/o3de/Gems/Atom/TestData/TestData/LightingPresets/beach_parking.lightingpreset.azasset"
    """
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(
        azlmbr.bus.Broadcast, "GetLastLightingPresetPathWithoutAlias")


def get_last_model_preset_path() -> str:
    """
    Returns the full path to the currently selected viewport model.
    Example return values observed when selecting different models from the vioewport dropdown:
    "C:/git/o3de/Gems/Atom/Tools/MaterialEditor/Assets/MaterialEditor/ViewportModels/Shaderball.modelpreset.azasset"
    "C:/git/o3de/Gems/Atom/Tools/MaterialEditor/Assets/MaterialEditor/ViewportModels/Cone.modelpreset.azasset"
    "C:/git/o3de/Gems/Atom/Tools/MaterialEditor/Assets/MaterialEditor/ViewportModels/BeveledCone.modelpreset.azasset"
    """
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(
        azlmbr.bus.Broadcast, "GetLastModelPresetPathWithoutAlias")


def get_last_model_preset_asset_id() -> azlmbr.math.Uuid:
    """
    Returns the asset ID of the currently selected viewport model.
    Example return values observed when selecting different models from the viewport dropdown:
    {9B61FAD7-2717-528C-9EBF-C1324D46ED56}:0
    {56C02199-7B4F-5896-A713-F70E6EBA0726}:0
    {46D4B53F-A900-591B-B4CD-75A79E47749B}:0
    """
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "GetLastModelPresetAssetId")


def set_grid_enabled(value: bool) -> None:
    azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "SetGridEnabled", value)


def get_grid_enabled() -> bool:
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "GetGridEnabled")


def set_shadow_catcher_enabled(value: bool) -> None:
    azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "SetShadowCatcherEnabled", value)


def get_shadow_catcher_enabled() -> bool:
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "GetShadowCatcherEnabled")


def load_model_preset_by_asset_id(asset_id: azlmbr.math.Uuid) -> bool:
    """
    Takes in an asset ID and attempts to select the model in the viewport dropdown using that ID.
    Returns True if it successfully changes, False otherwise.
    """
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(
        azlmbr.bus.Broadcast, "LoadModelPresetByAssetId", asset_id)


def load_model_preset_by_path(asset_path: str) -> bool:
    """
    Takes in an asset path and attempts to select the model in the viewport dropdown using that path.
    Returns True if it successfully changes it, False otherwise.
    """
    return azlmbr.atomtools.EntityPreviewViewportSettingsRequestBus(azlmbr.bus.Broadcast, "LoadModelPreset", asset_path)


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
