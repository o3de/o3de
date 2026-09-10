#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

"""Console output for the Class Creation Wizard.

The wizard prints into two very different consoles:

  * A terminal, when launched standalone through the engine's python.
  * The O3DE editor console, when launched from File -> New Component. There the
    editor swaps sys.stdout for a shim (RedirectOutput in
    Gems/EditorPythonBindings/Code/Source/PythonSystemComponent.cpp) that
    forwards EVERY write() call straight to the trace bus as its own console
    entry, which CCryEditApp::PythonOutputHandler in Code/Editor/CryEdit.cpp
    then tags with one hardcoded window name shared by all Python in the editor.

Two consequences shape this module:

  1. Wizard output is indistinguishable from any other script's, so every line
     carries a "[ClassWizard]" tag.
  2. print() writes the text and its trailing newline as two separate write()
     calls, so in the editor every message is followed by a blank console entry.
     In the editor the newline is left off instead, making one write() carry
     exactly one console line.
"""

import sys

_TAG = "[ClassWizard]"


def _running_in_editor() -> bool:
    """True when hosted by the editor's embedded interpreter. azlmbr is the
    editor's Python binding module and exists in no other interpreter."""
    try:
        import azlmbr  # noqa: F401
        return True
    except Exception:
        return False


IN_EDITOR = _running_in_editor()


def _emit(stream, message: str) -> None:
    """Write 'message' as one tagged console line per line of text."""
    for line in str(message).splitlines() or [""]:
        text = f"{_TAG} {line}" if line else _TAG
        stream.write(text if IN_EDITOR else text + "\n")


def wizard_log(message: str) -> None:
    """Report wizard status. This is the default logger for every wizard
    component, so status reaches whichever console the wizard was launched in."""
    _emit(sys.stdout, message)


def wizard_error(message: str) -> None:
    """As wizard_log, but on stderr, which the editor surfaces as an error."""
    _emit(sys.stderr, message)
