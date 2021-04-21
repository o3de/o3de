"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
import time
import azlmbr.legacy.general as general
import azlmbr.atom


class FailFast(BaseException):
    """
    Raise to stop proceeding through test steps.
    """

    pass


class TestHelper:
    @staticmethod
    def init_idle():
        general.idle_enable(True)
        general.idle_wait_frames(1)

    @staticmethod
    def open_level(level):
        # type: (str, ) -> None
        """
        :param level: the name of the level folder in MestTest\\

        :return: None
        """
        result = general.open_level(level)  # TO-DO: Check if success opening level
        if result:
            Report.info("Open level {}".format(level))
        else:
            Report.failure("Assert: failed to open level {}".format(level))
        general.idle_wait_frames(1)

    @staticmethod
    def enter_game_mode(msgtuple_success_fail):
        # type: (tuple) -> None
        """
        :param msgtuple_success_fail: The tuple with the expected/unexpected messages for entering game mode.

        :return: None
        """
        Report.info("Entering game mode")
        general.enter_game_mode()
        general.idle_wait_frames(1)
        Report.critical_result(msgtuple_success_fail, general.is_in_game_mode())

    @staticmethod
    def exit_game_mode(msgtuple_success_fail):
        # type: (tuple) -> None
        """
        :param msgtuple_success_fail: The tuple with the expected/unexpected messages for exiting game mode.

        :return: None
        """
        general.exit_game_mode()
        general.idle_wait_frames(1)
        Report.critical_result(msgtuple_success_fail, not general.is_in_game_mode())

    @staticmethod
    def close_editor():
        general.exit_no_prompt()

    @staticmethod
    def fail_fast(message=None):
        # type: (str) -> None
        """
        A state has been reached where progressing in the test is not viable.
        raises FailFast
        :return: None
        """
        Report.info("Failing fast. Raising an exception and shutting down the editor.")
        if message:
            Report.info("Fail fast message: {}".format(message))
        TestHelper.close_editor()
        raise FailFast()

    @staticmethod
    def wait_for_condition(function, timeout_in_seconds=2.0):
        # type: (function, float) -> bool
        """
        **** Will be replaced by a function of the same name exposed in the Engine*****
        a function to run until it returns True or timeout is reached
        the function can have no parameters and
        waiting idle__wait_* is handled here not in the function

        :param function: a function that returns a boolean indicating a desired condition is achieved
        :param timeout_in_seconds: when reached, function execution is abandoned and False is returned
        """

        with Timeout(timeout_in_seconds) as t:
            while True:
                general.idle_wait(1.0)
                if t.timed_out:
                    return False

                ret = function()
                if not isinstance(ret, bool):
                    raise TypeError("return value for wait_for_condition function must be a bool")
                if ret:
                    return True

    @staticmethod
    def find_entities(entity_name):
        search_filter = azlmbr.entity.SearchFilter()
        search_filter.names = [entity_name]
        searched_entities = azlmbr.entity.SearchBus(azlmbr.bus.Broadcast, 'SearchEntities', search_filter)
        return searched_entities

    @staticmethod
    def attach_component_to_entity(entityId, componentName):
        # type: (azlmbr.entity.EntityId, str) -> azlmbr.entity.EntityComponentIdPair
        """
        Adds the component if not added already.
        If successful, returns the EntityComponentIdPair, otherwise returns None.
        """
        typeIdsList = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'FindComponentTypeIdsByEntityType',
                                                          [componentName], 0)
        general.log("Components found = {}".format(len(typeIdsList)))
        if len(typeIdsList) < 1:
            general.log(f"ERROR: A component class with name {componentName} doesn't exist")
            return None
        elif len(typeIdsList) > 1:
            general.log(f"ERROR: Found more than one component classes with same name: {componentName}")
            return None
        # Before adding the component let's check if it is already attached to the entity.
        componentOutcome = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'GetComponentOfType', entityId,
                                                               typeIdsList[0])
        if componentOutcome.IsSuccess():
            return componentOutcome.GetValue()  # In this case the value is not a list.
        componentOutcome = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'AddComponentsOfType', entityId,
                                                               typeIdsList)
        if componentOutcome.IsSuccess():
            general.log(f"{componentName} Component added to entity.")
            return componentOutcome.GetValue()[0]
        general.log(f"ERROR: Failed to add component [{componentName}] to entity")
        return None

    @staticmethod
    def get_component_property(component, propertyPath):
        return azlmbr.editor.EditorComponentAPIBus(
            azlmbr.bus.Broadcast,
            'GetComponentProperty',
            component,
            propertyPath)

    @staticmethod
    def set_component_property(component, propertyPath, value):
        azlmbr.editor.EditorComponentAPIBus(
            azlmbr.bus.Broadcast,
            'SetComponentProperty',
            component,
            propertyPath,
            value)

    @staticmethod
    def get_property_list(Component):
        property_list = azlmbr.editor.EditorComponentAPIBus(azlmbr.bus.Broadcast, 'BuildComponentPropertyList',
                                                            Component)
        return property_list

    @staticmethod
    def compare_property_list(Component, PropertyList):
        property_list = TestHelper.get_property_list(Component)
        if set(property_list) == set(PropertyList):
            general.log("Property list of component is correct.")

    @staticmethod
    def isclose(a: float, b: float, rel_tol: float = 1e-9, abs_tol: float = 0.0) -> bool:
        return abs(a - b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol)


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


# NOTE: implementation of reports will be changed to use a better mechanism rather than print


class Report:
    @staticmethod
    def info(msg):
        print("Info: {}".format(msg))

    @staticmethod
    def success(msgtuple_success_fail):
        print("Success: {}".format(msgtuple_success_fail[0]))

    @staticmethod
    def failure(msgtuple_success_fail):
        print("Failure: {}".format(msgtuple_success_fail[1]))

    @staticmethod
    def result(msgtuple_success_fail, condition):
        if not isinstance(condition, bool):
            raise TypeError("condition argument must be a bool")

        if condition:
            Report.success(msgtuple_success_fail)
        else:
            Report.failure(msgtuple_success_fail)
        return condition

    @staticmethod
    def critical_result(msgtuple_success_fail, condition, fast_fail_message=None):
        # type: (tuple, bool, str) -> None
        """
        if condition is False we will fail fast

        :param msgtuple_success_fail: messages to print based on the condition
        :param condition: success (True) or failure (False)
        :param fast_fail_message: [optional] message to include on fast fail
        """
        if not isinstance(condition, bool):
            raise TypeError("condition argument must be a bool")

        if not Report.result(msgtuple_success_fail, condition):
            TestHelper.fail_fast(fast_fail_message)

    @staticmethod
    def info_vector3(vector3, label="", magnitude=None):
        # type: (azlmbr.math.Vector3, str, float) -> None
        """
        prints the vector to the Report.info log. If applied, label will print first,
        followed by the vector's values (x, y, z,) to 2 decimal places. Lastly if the
        magnitude is supplied, it will print on the third line.

        :param vector3: a azlmbr.math.Vector3 object to print
                    prints in [x: , y: , z: ] format.
        :param label: [optional] A string to print before printing the vector3's contents
        :param magnitude: [optional] the vector's magnitude to print after the vector's contents
        :return: None
        """
        if label != "":
            Report.info(label)
        Report.info("   x: {:.2f}, y: {:.2f}, z: {:.2f}".format(vector3.x, vector3.y, vector3.z))
        if magnitude is not None:
            Report.info("   magnitude: {:.2f}".format(magnitude))
