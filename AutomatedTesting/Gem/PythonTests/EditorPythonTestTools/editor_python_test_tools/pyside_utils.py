"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.qt
import azlmbr.qt_helpers
import asyncio
import re
from shiboken2 import wrapInstance, getCppPointer
from PySide2 import QtCore, QtWidgets, QtGui, QtTest
from PySide2.QtWidgets import QAction, QWidget
from PySide2.QtCore import Qt
from PySide2.QtTest import QTest
import traceback
import threading
import types


qApp = QtWidgets.QApplication.instance()


class LmbrQtEventLoop(asyncio.AbstractEventLoop):
    def __init__(self):
        self.running = False
        self.shutdown = threading.Event()
        self.blocked_events = set()
        self.finished_events = set()
        self.queue = []
        self._wait_future = None
        self._event_loop_nesting = 0

    def get_debug(self):
        return False

    def time(self):
        return azlmbr.qt_helpers.time()

    def wait_for_condition(self, condition, action, on_timeout=None, timeout=1.0):
        timeout = self.time() + timeout if timeout is not None else None
        def callback(time):
            # Run our action and remove us from the queue if our condition is satisfied
            if condition():
                action()
                return True
            # Give up if timeout has elapsed
            if time > timeout:
                if on_timeout is not None:
                    on_timeout()
                return True
            return False
        self.queue.append((callback))

    def event_loop(self):
        time = self.time()
        def run_event(event):
            if event in self.blocked_events or event in self.finished_events:
                return False
            self.blocked_events.add(event)
            try:
                if event(time):
                    self.finished_events.add(event)
            except Exception:
                traceback.print_exc()
                self.finished_events.add(event)
            finally:
                self.blocked_events.remove(event)

        self._event_loop_nesting += 1
        try:
            for event in self.queue:
                run_event(event)
        finally:
            self._event_loop_nesting -= 1

        # Clear out any finished events if the queue is safe to mutate
        if self._event_loop_nesting == 0:
            self.queue = [event for event in self.queue if event not in self.finished_events]
            self.finished_events = set()

        if not self.running or self._wait_future is not None and self._wait_future.done():
            self.close()

    def run_until_shutdown(self):
        # Run our event loop callback (via azlmbr.qt_helpers) by pumping the Qt event loop
        # azlmbr.qt_helpers will attempt to ensure our event loop is always run, even when a
        # new event loop is started and run from the main event loop
        self.running = True
        self.shutdown.clear()
        azlmbr.qt_helpers.set_loop_callback(self.event_loop)
        while not self.shutdown.is_set():
            qApp.processEvents(QtCore.QEventLoop.AllEvents, 0)

    def run_forever(self):
        self._wait_future = None
        self.run_until_shutdown()

    def run_until_complete(self, future):
        # Wrap coroutines into Tasks (future-like analogs)
        if isinstance(future, types.CoroutineType):
            future = self.create_task(future)
        self._wait_future = future
        self.run_until_shutdown()

    def _timer_handle_cancelled(self, handle):
        pass

    def is_running(self):
        return self.running

    def is_closed(self):
        return not azlmbr.qt_helpers.loop_is_running()

    def stop(self):
        self.running = False

    def close(self):
        self.running = False
        self.shutdown.set()
        azlmbr.qt_helpers.clear_loop_callback()

    def shutdown_asyncgens(self):
        pass

    def call_exception_handler(self, context):
        try:
            raise context.get('exception', None)
        except:
            traceback.print_exc()

    def call_soon(self, callback, *args, **kw):
        h = asyncio.Handle(callback, args, self)
        def callback_wrapper(time):
            if not h.cancelled():
                h._run()
            return True
        self.queue.append(callback_wrapper)
        return h

    def call_later(self, delay, callback, *args, **kw):
        if delay < 0:
            raise Exception("Can't schedule in the past")
        return self.call_at(self.time() + delay, callback, *args)

    def call_at(self, when, callback, *args, **kw):
        h = asyncio.TimerHandle(when, callback, args, self)
        h._scheduled = True
        def callback_wrapper(time):
            if time > when:
                if not h.cancelled():
                    h._run()
                return True
            return False
        self.queue.append(callback_wrapper)
        return h

    def create_task(self, coro):
        return asyncio.Task(coro, loop=self)

    def create_future(self):
        return asyncio.Future(loop=self)


class EventLoopTimeoutException(Exception):
    pass


event_loop = LmbrQtEventLoop()
def wait_for_condition(condition, timeout=1.0):
    """
    Asynchronously waits for `condition` to evaluate to True.
    condition: A function with the signature def condition() -> bool
    This condition will be evaluated until it evaluates to True or the timeout elapses
    timeout: The time in seconds to wait - if 0, this will wait forever
    Throws pyside_utils.EventLoopTimeoutException on timeout.
    """
    future = event_loop.create_future()
    def on_complete():
        future.set_result(True)
    def on_timeout():
        future.set_exception(EventLoopTimeoutException())
    event_loop.wait_for_condition(condition, on_complete, on_timeout=on_timeout, timeout=timeout)
    return future


async def wait_for(expression, timeout=1.0):
    """
    Asynchronously waits for "expression" to evaluate to a non-None value,
    then returns that value.

    expression: A function with the signature def expression() -> Generic[Any,None]
    The result of expression will be returned as soon as it returns a non-None value.
    timeout: The time in seconds to wait - if 0, this will wait forever
    Throws pyside_utils.EventLoopTimeoutException on timeout.
    """
    result = None
    def condition():
        nonlocal result
        result = expression()
        return result is not None
    await wait_for_condition(condition, timeout)
    return result


def run_soon(fn):
    """
    Runs a function on the event loop to enable asynchronous execution.

    fn: The function to run, should be a function that takes no arguments
    Returns a future that will be popualted with the result of fn or the exception it threw.
    """
    future = event_loop.create_future()
    def coroutine():
        try:
            fn()
            future.set_result(True)
        except Exception as e:
            future.set_exception(e)
    event_loop.call_soon(coroutine)
    return future


def run_async(awaitable):
    """
    Synchronously runs a coroutine or a future on the event loop.
    This can be used in lieu of "await" in non-async functions.

    awaitable: The coroutine or future to await.
    Returns the result of operation specified.
    """
    if isinstance(awaitable, types.CoroutineType):
        awaitable = event_loop.create_task(awaitable)
    event_loop.run_until_complete(awaitable)
    return awaitable.result()


def wrap_async(fn):
    """
    This decorator enables an async function's execution from a synchronous one.

    For example:
    @pyside_utils.wrap_async
    async def foo():
        result = await long_operation()
        return result

    def non_async_fn():
        x = foo() # this will return the correct result by executing the event loop

    fn: The function to wrap
    Returns the decorated function.
    """
    def wrapper(*args, **kw):
        result = fn(*args, **kw)
        return run_async(result)
    return wrapper


def get_editor_main_window():
    """
    Fetches the main Editor instance of QMainWindow for use with PySide tests
    :return Instance of QMainWindow for the Editor
    """
    params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, "GetQtBootstrapParameters")
    editor_id = QtWidgets.QWidget.find(params.mainWindowId)
    main_window = wrapInstance(int(getCppPointer(editor_id)[0]), QtWidgets.QMainWindow)
    return main_window


def get_action_for_menu_path(editor_window: QtWidgets.QMainWindow, main_menu_item: str, *menu_item_path: str):
    """
    main_menu_item: Main menu item among the MenuBar actions. Ex: "File"
    menu_item_path: Path to any nested menu item. Ex: "Viewport", "Goto Coordinates"
    returns: QAction object for the corresponding path.
    """
    # Check if path is valid
    menu_bar = editor_window.menuBar()
    menu_bar_actions = [index.iconText() for index in menu_bar.actions()]

    # Verify if the given Menu exists in the Menubar
    if main_menu_item not in menu_bar_actions:
        print(f"QAction not found for main menu item '{main_menu_item}'")
        return None
    curr_action = menu_bar.actions()[menu_bar_actions.index(main_menu_item)]
    curr_menu = curr_action.menu()
    for index, element in enumerate(menu_item_path):
        curr_menu_actions = [index.iconText() for index in curr_menu.actions()]
        if element not in curr_menu_actions:
            print(f"QAction not found for menu item '{element}'")
            return None
        if index == len(menu_item_path) - 1:
            return curr_menu.actions()[curr_menu_actions.index(element)]
        curr_action = curr_menu.actions()[curr_menu_actions.index(element)]
        curr_menu = curr_action.menu()
    return None


def _pattern_to_dict(pattern, **kw):
    """
    Helper function, turns a pattern match parameter into a normalized dictionary
    """

    def is_string_or_regex(x):
        return isinstance(x, str) or isinstance(x, re.Pattern)

    # If it's None, just make an empty dict
    if pattern is None:
        pattern = {}
    # If our pattern is a string or regex, turn it into a text match
    elif is_string_or_regex(pattern):
        pattern = dict(text=pattern)
    # If our pattern is an (int, int) tuple, turn it into a row/column match
    elif isinstance(pattern, tuple) and isinstance(pattern[0], int) and isinstance(pattern[1], int):
        pattern = dict(row=pattern[0], column=pattern[1])
    # If our pattern is a QObject type, turn it into a type match
    elif isinstance(pattern, type(QtCore.QObject)):
        pattern = dict(type=pattern)
    # Otherwise assume it's a dict and make a copy
    else:
        pattern = dict(pattern)

    # Merge with any kw arguments
    for key, value in kw.items():
        pattern[key] = value
    return pattern


def _match_pattern(obj, pattern):
    """
    Helper function, determines whether obj matches the pattern specified by pattern.

    It is required that pattern is normalized into a dict before calling this.
    """

    def compare(value1, value2):
        # Do a regex search if it's a regex, otherwise do a normal compare
        if isinstance(value2, re.Pattern):
            return re.search(value2, value1)
        return value1 == value2

    item_roles = Qt.ItemDataRole.values.values()
    for key, value in pattern.items():
        if key == "type":  # Class type
            if not isinstance(obj, value):
                return False
        elif key == "text":  # Default 'text' path, depends on type
            text_values = []

            def get_from_attrs(*args):
                for attr in args:
                    try:
                        text_values.append(getattr(obj, attr)())
                    except Exception:
                        pass

            # Use any of the following fields for default matching, if they're defined
            get_from_attrs("text", "objectName", "windowTitle")
            # Additionally, use the DisplayRole for QModelIndexes
            if isinstance(obj, QtCore.QModelIndex):
                text_values.append(obj.data(Qt.DisplayRole))

            if not any(compare(text, value) for text in text_values):
                return False
        elif key in item_roles:  # QAbstractItemModel display role
            if not isinstance(obj, QtCore.QModelIndex):
                raise RuntimeError(f"Attempted to match data role on unsupported object {obj}")
            if not compare(obj.data(key), value):
                return False
        elif hasattr(obj, key):
            # Look up our key on the object itself
            objectValue = getattr(obj, key)
            # Invoke it if it's a getter
            if callable(objectValue):
                objectValue = objectValue()
            if not compare(objectValue, value):
                return False
        else:
            return False

    return True


def get_child_indexes(model, parent_index=QtCore.QModelIndex()):
    indexes = [parent_index]
    while len(indexes) > 0:
        parent_index = indexes.pop(0)
        for row in range(model.rowCount(parent_index)):
            # FIXME
            # PySide appears to have a bug where-in it thinks columnCount is private
            # Bail gracefully for now, we can add a C++ wrapper to work around if needed
            try:
                column_count = model.columnCount(parent_index)
            except Exception:
                column_count = 1
            for col in range(column_count):
                cur_index = model.index(row, col, parent_index)
                yield cur_index


def _get_children(obj):
    """
    Helper function. Get the direct descendants from a given PySide object.
    This includes all: QObject children, QActions owned by the object, and QModelIndexes if applicable
    """
    if isinstance(obj, QtCore.QObject):
        yield from obj.children()
    if isinstance(obj, QtWidgets.QWidget):
        yield from obj.actions()
    if isinstance(obj, (QtWidgets.QAbstractItemView, QtCore.QModelIndex)):
        model = obj.model()
        if model is None:
            return

        # For a QAbstractItemView (e.g. QTreeView, QListView), the parent index
        # will be an invalid QModelIndex(), which will use find all indexes on the root.
        # For a QModelIndex, we use the actual QModelIndex as the parent_index so that
        # it will find any child indexes under it
        parent_index = QtCore.QModelIndex()
        if isinstance(obj, QtCore.QModelIndex):
            parent_index = obj

        yield from get_child_indexes(model, parent_index)


def _get_parents_to_search(obj_entry_or_list):
    """
    Helper function, turns obj_entry_or_list into a list of parents to search

    If obj_entry_or_list is None, returns all visible top level widgets
    If obj_entry_or_list is iterable, return it as a list
    Otherwise, return a list containing obj_entry_or_list
    """
    if obj_entry_or_list is None:
        return [widget for widget in QtWidgets.QApplication.topLevelWidgets() if widget.isVisible()]
    try:
        return list(obj_entry_or_list)
    except TypeError:
        return [obj_entry_or_list]


def find_children_by_pattern(obj=None, pattern=None, recursive=True, **kw):
    """
    Finds the children of an object that match a given pattern.
    See find_child_by_pattern for more information on usage.
    """
    pattern = _pattern_to_dict(pattern, **kw)
    parents_to_search = _get_parents_to_search(obj)

    while len(parents_to_search) > 0:
        parent = parents_to_search.pop(0)
        for child in _get_children(parent):
            if _match_pattern(child, pattern):
                yield child
            if recursive:
                parents_to_search.append(child)


def find_child_by_pattern(obj=None, pattern=None, recursive=True, **kw):
    """
    Finds the child of an object that matches a given pattern.
    A "child" in this context is not necessarily a QObject child.
    QActions are also considered children, as are the QModelIndex children of QAbstractItemViews.
    obj: The object to search - should be either a QObject or a QModelIndex, or a list of them
    If None this will search all top level windows.
    pattern: The pattern to match, the first child that matches all of the criteria specified will
    be returned. This is a dictionary with any combination of the following:

    - "text": generic text to match, will search object names for QObjects, display role text
      for QModelIndexes, or action text() for QActions
    - "type": a class type, e.g. QtWidgets.QMenu, a child will only match if it's of this type
    - "row" / "column": integer row and column indices of a QModelIndex
    - "type": type class (e.g. PySide.QtWidgets.QComboBox) that the object must inherit from
    - A Qt.ItemDataRole: matches for QModelIndexes with data of a given value
    - Any other fields will fall back on being looked up on the object itself by name, e.g.
      {"windowTitle": "Foo"} would match a windowTitle named "Foo"

    Any instances where a field is specified as text can also be specified as a regular expression:
    find_child_by_pattern(obj, {text: re.compile("Foo_.*")}) would find a child with text starting
    with "Foo_"

    For convenience, these parameter types may also be specified as keyword arguments:
    find_child_by_pattern(obj, text="foo", type=QtWidgets.QAction)
    is equivalent to
    find_child_by_pattern(obj, {"text": "foo", "type": QtWidgets.QAction})

    If pattern is specified as a string, it will turn into a pattern matching "text":
    find_child_by_pattern(obj, "foo")
    is equivalent to
    find_child_by_pattern(obj, {"text": "foo"})

    If a pattern is specified as an (int, int) tuple, it will turn into a row/column match:
    find_child_by_pattern(obj, (0, 2))
    is equivalent to
    find_child_by_pattern(obj, {"row": 0, "column": 2})

    If a pattern is specified as a type, like PySide.QtWidgets.QLabel, it will turn into a type match:
    find_child_by_pattern(obj, PySide.QtWidgets.QLabel)
    is equivalent to
    find_child_by_pattern(obj, {"type": PySide.QtWidgets.QLabel})
    """
    # Return the first match result, if found
    for match in find_children_by_pattern(obj, pattern=pattern, recursive=recursive, **kw):
        return match
    return None


def find_child_by_hierarchy(parent, *patterns):
    """
    Searches for a hierarchy of children descending from parent.
    parent: The Qt object (or list of Qt obejcts) to search within
    If none, this will search all top level windows.
    patterns: A list of patterns to match to find a hierarchy of descendants.
    These patterns will be tested in order.

    For example, to look for the QComboBox in a hierarchy like the following:
    QWidget (window)
      -QTabWidget
        -QWidget named "m_exampleTab"
          -QComboBox
    One might invoke:
    find_child_by_hierarchy(window, QtWidgets.QTabWidget, "m_exampleTab", QtWidgets.QComboBox)

    Alternatively, "..." may be specified in place of a parent, where the hierarchy will match any
    ancestors along the path, so the above might be shortened to:
    find_child_by_hierarchy(window, ..., "m_exampleTab", QtWidgets.QComboBox)
    """
    search_recursively = False
    current_objects = _get_parents_to_search(parent)
    for pattern in patterns:
        # If it's an ellipsis, do the next search recursively as we're looking for any number of intermediate ancestors
        if pattern is ...:
            search_recursively = True
            continue

        candidates = []
        for parent_candidate in current_objects:
            candidates += find_children_by_pattern(parent_candidate, pattern=pattern, recursive=search_recursively)
        if len(candidates) == 0:
            return None
        current_objects = candidates

        search_recursively = False
    return current_objects[0]

async def wait_for_child_by_hierarchy(parent, *patterns, timeout=1.0):
    """
    Searches for a hierarchy of children descending from parent until timeout occurs.
    Returns a future that will result in either the found child or an EventLoopTimeoutException.

    See find_child_by_hierarchy for usage information.
    """
    match = None
    def condition():
        nonlocal match
        match = find_child_by_hierarchy(parent, *patterns)
        return match is not None
    await wait_for_condition(condition, timeout)
    return match


async def wait_for_child_by_pattern(obj=None, pattern=None, recursive=True, timeout=1.0, **kw):
    """
    Finds the child of an object that matches a given pattern.
    Returns a future that will result in either the found child or an EventLoopTimeoutException.

    See find_child_by_hierarchy for usage information.
    """
    match = None
    def condition():
        nonlocal match
        match = find_child_by_pattern(obj, pattern, recursive, **kw)
        return match is not None
    await wait_for_condition(condition, timeout)
    return match


def find_child_by_property(obj, obj_type, property_name, property_value, reg_exp_search=False):
    """
    Finds the child of an object which has the property name matching the property value
    of type obj_type
    obj: The property value is searched through obj children
    obj_type: Type of object to be matched
    property_name: Property of the child which should be verified for the required value.
    property_value: Property value that needs to be matched
    reg_exp_search: If True searches for the property_value based on re search. Defaults to False.
    """
    for child in obj.children():
        if reg_exp_search and re.search(property_value, getattr(child, property_name)()):
            return child
        if not reg_exp_search and isinstance(child, obj_type) and getattr(child, property_name)() == property_value:
            return child
    return None

def get_item_view_index(item_view, row, column=0, parent=QtCore.QModelIndex()):
    """
    Retrieve the index for a specified row/column, with optional parent
    This is necessary when needing to reference into nested hierarchies in a QTreeView
    item_view: The QAbstractItemView instance
    row: The requested row index
    column: The requested column index (defaults to 0 in case of single column)
    parent: Parent index (defaults to invalid)
    """
    item_model = item_view.model()
    model_index = item_model.index(row, column, parent)
    return model_index


def get_item_view_index_rect(item_view, index):
    """
    Gets the QRect for a given index in a QAbstractItemView (e.g. QTreeView, QTableView, QListView).
    This is helpful because for sending mouse events to a QAbstractItemView, you have to send them to
    the viewport() widget of the QAbstractItemView.
    item_view: The QAbstractItemView instance
    index: A QModelIndex for the item index
    """
    return item_view.visualRect(index)


def item_view_index_mouse_click(item_view, index, button=QtCore.Qt.LeftButton, modifier=QtCore.Qt.NoModifier):
    """
    Helper method version of QTest.mouseClick for injecting mouse clicks on a QAbstractItemView
    item_view: The QAbstractItemView instance
    index: A QModelIndex for the item index to be clicked
    """
    item_index_rect = get_item_view_index_rect(item_view, index)
    item_index_center = item_index_rect.center()

    # For QAbstractItemView widgets, the events need to be forwarded to the actual viewport() widget
    QTest.mouseClick(item_view.viewport(), button, modifier, item_index_center)


def item_view_mouse_click(item_view, row, column=0, button=QtCore.Qt.LeftButton, modifier=QtCore.Qt.NoModifier):
    """
    Helper method version of 'item_view_index_mouse_click' using a row, column instead of a QModelIndex
    item_view: The QAbstractItemView instance
    row: The requested row index
    column: The requested column index (defaults to 0 in case of single column)
    """
    index = get_item_view_index(item_view, row, column)
    item_view_index_mouse_click(item_view, index, button, modifier)


async def wait_for_action_in_menu(menu, pattern, timeout=1.0):
    """
    Finds a QAction inside a menu, based on the specified pattern.

    menu: The QMenu to search
    pattern: The action text or pattern to match (see find_child_by_pattern)
    If pattern specifies a QWidget, this will search for the associated QWidgetAction
    """
    action = await wait_for_child_by_pattern(menu, pattern, timeout=timeout)
    if action is None:
        raise TimeoutError(f"Failed to find context menu action for {pattern}")

    # If we've found a valid QAction, we're good to go
    if hasattr(action, 'trigger'):
        return action

    # If pattern matches a widget and not a QAction, look for an associated QWidgetAction
    widget_actions = find_children_by_pattern(menu, type=QtWidgets.QWidgetAction)
    underlying_widget_action = None
    for widget_action in widget_actions:
        widgets_to_check = [widget_action.defaultWidget()] + widget_action.createdWidgets()
        for check_widget in widgets_to_check:
            if action in _get_children(check_widget):
                underlying_widget_action = widget_action
                break
        if underlying_widget_action is not None:
            action = underlying_widget_action
            break

    if not hasattr(action, 'trigger'):
        raise RuntimeError(f"Failed to find action associated with widget {action}")
    return action


def queue_hide_event(widget):
    """
    Explicitly post a hide event for the next frame, this can be used to ensure modal dialogs exit correctly.

    widget: The widget to hide
    """
    qApp.postEvent(widget, QtGui.QHideEvent())


async def wait_for_destroyed(obj, timeout=1.0):
    """
    Waits for a QObject (including a widget) to be fully destroyed

    This can be used to wait for a modal dialog to shut down properly

    obj: The object to wait on destruction
    timeout: The time, in seconds to wait. 0 for an indefinite wait.
    """
    was_destroyed = False
    def on_destroyed():
        nonlocal was_destroyed
        was_destroyed = True
    obj.destroyed.connect(on_destroyed)
    return await wait_for_condition(lambda: was_destroyed, timeout=timeout)


async def close_modal(modal_widget, timeout=1.0):
    """
    Closes a modal dialog and waits for it to be cleaned up.

    This attempts to ensure the modal event loop gets properly exited.

    modal_widget: The widget to close
    timeout: The time, in seconds, to wait. 0 for an indefinite wait.
    """
    queue_hide_event(modal_widget)
    return await wait_for_destroyed(modal_widget, timeout=timeout)


def trigger_context_menu_entry(widget, pattern, pos=None, index=None):
    """
    Trigger a context menu event on a widget and activate an entry
    widget: The widget to trigger the event on
    pattern: The action text or pattern to match (see find_child_by_pattern)
    pos: Optional, the QPoint to set as the event origin
    index: Optional, the QModelIndex to click in widget
    widget must be a QAbstractItemView
    """
    async def async_wrapper():
        menu = await open_context_menu(widget, pos=pos, index=index)
        action = await wait_for_action_in_menu(menu, pattern)
        action.trigger()
        queue_hide_event(menu)

    result = async_wrapper()
    # If we have an event loop, go ahead and just return the coroutine
    # Otherwise, do a synchronous wait
    if event_loop.is_running():
        return result
    else:
        return run_async(result)


async def open_context_menu(widget, pos=None, index=None, timeout=1.0):
    """
    Trigger a context menu event on a widget
    widget: The widget to trigger the event on
    pos: Optional, the QPoint to set as the event origin
    index: Optional, the QModelIndex to click in widget
    widget must be a QAbstractItemView

    Returns the menu that was created.
    """
    if index is not None:
        if pos is not None:
            raise RuntimeError("Error: 'index' and 'pos' are mutually exclusive")
        pos = widget.visualRect(index).center()
        parent = widget
        widget = widget.viewport()
        pos = widget.mapFrom(parent, pos)
    if pos is None:
        pos = widget.rect().center()

    # Post both a mouse event and a context menu to let the widget handle whichever is appropriate
    qApp.postEvent(widget, QtGui.QContextMenuEvent(QtGui.QContextMenuEvent.Mouse, pos))
    QtTest.QTest.mouseClick(widget, Qt.RightButton, Qt.NoModifier, pos)

    menu = None
    # Wait for a menu popup
    def menu_has_focus():
        nonlocal menu
        for fw in [QtWidgets.QApplication.activePopupWidget(), QtWidgets.QApplication.activeModalWidget(),
                   QtWidgets.QApplication.focusWidget(), QtWidgets.QApplication.activeWindow()]:
            if fw and isinstance(fw, QtWidgets.QMenu) and fw.isVisible():
                menu = fw
                return True
        return False
    await wait_for_condition(menu_has_focus, timeout)
    return menu


def move_mouse(widget, position):
    """
    Helper method to move the mouse to a specified position on a widget
    widget: The widget to trigger the event on
    position: The QPoint (local to widget) to move the mouse to
    """
    # For some reason, Qt wouldn't register the mouse movement correctly unless both of these ways are invoked.
    # The QTest.mouseMove seems to update the global cursor position, but doesn't always result in the MouseMove event being
    # triggered, which prevents drag/drop being able to be simulated.
    # Similarly, if only the MouseMove event is sent by itself to the core application, the global cursor position wasn't
    # updated properly, so drag/drop logic that depends on grabbing the globalPos didn't work.
    QtTest.QTest.mouseMove(widget, position)
    event = QtGui.QMouseEvent(QtCore.QEvent.MouseMove, position, widget.mapToGlobal(position), QtCore.Qt.LeftButton, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier)
    QtCore.QCoreApplication.sendEvent(widget, event)


def drag_and_drop(source, target, source_point = QtCore.QPoint(), target_point = QtCore.QPoint()):
    """
    Simulate a drag/drop event from a source object to a specified target
    This has special case handling if the source is a QDockWidget (for docking) vs normal drag/drop
    source: The source object to initiate the drag from
            This is either a QWidget, or a tuple of (QAbstractItemView, QModelIndex) for dragging an item view item
    target: The target object to drop on after dragging
            This is either a QWidget, or a tuple of (QAbstractItemView, QModelIndex) for dropping on an item view item
    source_point: Optional, The QPoint to initiate the drag from. If none is specified, the center of the source will be used.
    target_point: Optional, The QPoint to drop on. If none is specified, the center of the target will be used.
    """
    # Flag if this drag/drop is for docking, which has some special cases
    docking = False

    # If the source is a tuple of (QAbstractItemView, QModelIndex), we need to use the
    # viewport() as the source, and find the location of the index
    if isinstance(source, tuple) and len(source) == 2:
        source_item_view = source[0]
        source_widget = source_item_view.viewport()
        source_model_index = source[1]
        source_rect = source_item_view.visualRect(source_model_index)
    else:
        # There are some special case actions if we are doing this drag for docking,
        # so figure this out by checking if the source is a QDockWidget
        if isinstance(source, QtWidgets.QDockWidget):
            docking = True

        source_widget = source
        source_rect = source.rect()

    # If the target is a tuple of (QAbstractItemView, QModelIndex), we need to use the
    # viewport() as the target, and find the location of the index
    if isinstance(target, tuple) and len(target) == 2:
        target_item_view = target[0]
        target_widget = target_item_view.viewport()
        target_model_index = target[1]
        target_rect = target_item_view.visualRect(target_model_index)
    else:
        # If we are doing a drag for docking, we actually want all the mouse events
        # to still be directed through the source widget
        if docking:
            target_widget = source_widget
        else:
            target_widget = target
        target_rect = target.rect()

    # If no source_point is specified, we need to find the center point of
    # the source widget
    if source_point.isNull():
        # If we are dragging for docking, initiate the drag from the center of the
        # dock widget title bar
        if docking:
            title_bar_widget = source.titleBarWidget()
            if title_bar_widget:
                source_point = title_bar_widget.geometry().center()
            else:
                raise RuntimeError("No titleBarWidget found for QDockWidget")
        # Otherwise, can just find the center of the rect
        else:
            source_point = source_rect.center()

    # If no target_point was specified, we need to find the center point of the target widget
    if target_point.isNull():
        target_point = target_rect.center()

    # If we are dragging for docking and we aren't dragging within the same source/target,
    # the mouse movements need to be directed to the source_widget, so we need to use the
    # difference in global positions of our source and target widgets to adjust the target_point
    # to be relative to the source
    if docking and source != target:
        source_top_left = source.mapToGlobal(QtCore.QPoint(0, 0))
        target_top_left = target.mapToGlobal(QtCore.QPoint(0, 0))
        offset = target_top_left - source_top_left
        target_point += offset

    # Move the mouse to the source spot where we will start the drag
    move_mouse(source_widget, source_point)

    # Press the left-mouse button to begin the drag
    QtTest.QTest.mousePress(source_widget, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier, source_point)

    # If we are dragging for docking, we first need to drag the mouse past the minimum distance to
    # trigger the docking system properly
    if docking:
        drag_distance = QtWidgets.QApplication.startDragDistance() + 1
        docking_trigger_point = source_point + QtCore.QPoint(drag_distance, drag_distance)
        move_mouse(source_widget, docking_trigger_point)

    # Drag the mouse to the target widget over the desired point
    move_mouse(target_widget, target_point)

    # Release the left-mouse button to complete the drop.
    # If we are docking, we need to delay the actual mouse button release because the docking system has
    # a delay before the drop zone becomes active after it has been hovered, which can be found here:
    #     FancyDockingDropZoneConstants::dockingTargetDelayMS = 110 ms
    # So we need to delay greater than dockingTargetDelayMS after the final mouse move
    # over the intended target.
    delay = -1
    if docking:
        delay = 200
    QtTest.QTest.mouseRelease(target_widget, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier, target_point, delay)

    # Some drag/drop events have extra processing on the following event tick, so let those processEvents
    # first before we complete the drag/drop operation
    QtWidgets.QApplication.processEvents()


def trigger_action_async(action):
    """
    Convenience function. Triggers an action asynchronously.
    This can be used if calling action.trigger might block (e.g. if it opens a modal dialog)

    action: The action to trigger
    """
    return run_soon(lambda: action.trigger())


def click_button_async(button):
    """
    Convenience function. Clicks a button asynchronously.
    This can be used if calling button.click might block (e.g. if it opens a modal dialog)

    button: The button to click
    """
    return run_soon(lambda: button.click())


async def wait_for_modal_widget(timeout=1.0):
    """
    Waits for an active modal widget and returns it.
    """
    return await wait_for(lambda: QtWidgets.QApplication.activeModalWidget(), timeout=timeout)
    
async def wait_for_popup_widget(timeout=1.0):
    """
    Waits for an active popup widget and returns it.
    """
    return await wait_for(lambda: QtWidgets.QApplication.activePopupWidget(), timeout=timeout)