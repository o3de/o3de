Release Notes 1.0
=================

This gem adds an Editor-only system to reflect editor-flagged Behavior Class
methods to a Python VM running in the Lumberyard Editor. The gem will replace
the current Boost Python bindings in the Editor plus to allow gem authors to
reflect automation methods to Python for use in automate tests or workflows. It
encapsulates the Python Virtual Machine (VM) (currently at version 3.3) and uses
a modern C++ binding library named pybind11 to hook up the behavior methods to
Python.

Documentation
-------------

The gem works when a product team adds it to a project; this is not a required
gem. This will enable a system component that will enumerate all the
BehaviorClass methods registered with an “Editor” flag. The gem will also offer
new console commands to allow Editor users to execute automation scripts.

New Third Party Libraries

The gem includes two new 3rd Party Libraries:

- Python VM 3.3.5
    -   \$\\dev\\Gems\\EditorPythonBindings\\3rdParty\\python33
    -   Windows-Only

- PyBind11
    -   \$\\dev\\Gems\\EditorPythonBindings\\3rdParty\\pybind11
    -   <https://github.com/pybind/pybind11>
    -   Used for C++11 binding to the Behavior Class methods

### Reference

The gem comes with new EBuses and a console commands.

#### EditorPythonBindings.EditorPythonBindingsNotifications

The gem uses this EBus to send out notifications during the VM’s life cycle.
When another gem wants to add custom Python modules to the system it can respond
to these APIs to properly install or handle the Python environment.

The API:

-   OnPreInitialize() - sent when the Python VM is about to start right after the Editor has been initialized
-   OnPostInitialize() - sent when the Python VM properly initialized
-   OnPreFinalize() - sent when the Python VM is about to shut down; a good place to clean up resource borrowed from the Python VM
-   OnPostFinalize() - sent when the Python VM shut down cleanly
-   OnImportModule(module) - sent when any module is being imported from Python

#### AzToolsFramework.EditorPythonRunnerRequestBus

Coders use this bus to run Python scripts that execute in the
EditorPythonBinding gem. This bus provides API to run either from a file or from
a buffer.

The API:

- ExecuteByString(string) – runs a string buffer in the Python VM; it returns no value
- ExecuteByFilename(string) – loads a file off of the disk to execute in the
    Python VM; the call returns no value. The filename can contain an alias such as
    ‘\@projectroot\@’ to execute a project relative file inside the Editor

#### New Console Commands

The EditorPythonBindings gem offers two new console commands to the Editor:

- pyRunScript - console command is used to execute a script file from disk; it does not accept script arguments

