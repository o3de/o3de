"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    open_existing_material_graph = (
        "Existing material graph opened successfully.",
        "P0: Failed to open existing material graph.")
    close_opened_material_graph = (
        "Closed existing material graph.",
        "P0: Failed to close existing material graph.")
    close_all_opened_material_graphs = (
        "Closed all open material graphs.",
        "P0: Failed to close all open material graphs.")
    verify_all_material_graphs_are_opened = (
        "Verified all expected material graphs are opened.",
        "P0: Failed to verify all expected material graphs are opened.")
    close_all_opened_material_graphs_except_one = (
        "Closed all open material graphs except for one.",
        "P0: Failed to close all open material graphs except for one.")
    node_palette_pane_visible = (
        "Node Palette pane is visible.",
        "P0: Failed to verify Node Palette pane is visible.")
    material_graph_name_is_test1 = (
        "Verified material graph name is 'test1'",
        "P0: Failed to verify material graph name is 'test1'")


def MaterialCanvas_BasicFunctionalityChecks_AllChecksPass():
    """
    Summary:
    Tests basic MaterialCanvas functionality but does not deal with file I/O (i.e. saving).

    Expected Behavior:
    All MaterialCanvas basic tests pass.

    Test Steps:
    1) Open an existing material graph document.
    2) Close the selected material graph document.
    3) Open multiple material graph documents then use the CloseAllDocuments bus call to close them all.
    4) Open multiple material graph documents then verify all material documents are opened.
    5) Use the CloseAllDocumentsExcept bus call to close all but one.
    6) Verify Node Palette pane visibility.
    7) Verify material graph name is 'test1'.
    8) Look for errors and asserts.

    :return: None
    """

    import os

    import azlmbr.atomtools.util
    import Atom.atom_utils.atom_tools_utils as atom_tools_utils
    import Atom.atom_utils.material_canvas_utils as material_canvas_utils
    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    with Tracer() as error_tracer:
        # Disable automatic material and shader generation when opening graphs.
        azlmbr.atomtools.util.SetSettingsValue_bool("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnOpen", False)
        azlmbr.atomtools.util.SetSettingsValue_bool("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave", False)
        azlmbr.atomtools.util.SetSettingsValue_bool("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit", False)

        # Set constants before starting test steps.
        test_1_material_graph = os.path.join(atom_tools_utils.MATERIALCANVAS_GRAPH_PATH, "test1.materialgraph")
        test_2_material_graph = os.path.join(atom_tools_utils.MATERIALCANVAS_GRAPH_PATH, "test2.materialgraph")
        test_3_material_graph = os.path.join(atom_tools_utils.MATERIALCANVAS_GRAPH_PATH, "test3.materialgraph")
        test_4_material_graph = os.path.join(atom_tools_utils.MATERIALCANVAS_GRAPH_PATH, "test4.materialgraph")
        test_5_material_graph = os.path.join(atom_tools_utils.MATERIALCANVAS_GRAPH_PATH, "test5.materialgraph")

        # 1. Open an existing material graph document.
        material_document_id = atom_tools_utils.open_document(test_1_material_graph)
        Report.result(
            Tests.open_existing_material_graph,
            atom_tools_utils.is_document_open(material_document_id) is True)

        # 2. Close the selected material graph document.
        atom_tools_utils.close_document(material_document_id)
        Report.result(
            Tests.close_opened_material_graph,
            atom_tools_utils.is_document_open(material_document_id) is False)

        # 3. Open multiple material graph documents then use the CloseAllDocuments bus call to close them all.
        for material_graph_document in [test_1_material_graph, test_2_material_graph, test_3_material_graph,
                                        test_4_material_graph, test_5_material_graph]:
            atom_tools_utils.open_document(os.path.join(material_graph_document))
        Report.result(
            Tests.close_all_opened_material_graphs,
            atom_tools_utils.close_all_documents() is True)

        # 4. Open multiple material graph documents then verify all material documents are opened.
        material_graph_document_ids = []
        test_material_graphs = [test_1_material_graph, test_2_material_graph, test_3_material_graph,
                                test_4_material_graph, test_5_material_graph]
        for test_material_graph in test_material_graphs:
            material_graph_document_id = atom_tools_utils.open_document(test_material_graph)
            Report.result(Tests.verify_all_material_graphs_are_opened,
                          atom_tools_utils.is_document_open(material_graph_document_id) is True)
            material_graph_document_ids.append(material_graph_document_id)

        # 5. Use the CloseAllDocumentsExcept bus call to close all but one.
        atom_tools_utils.close_all_except_selected(material_graph_document_ids[0])
        Report.result(
            Tests.close_all_opened_material_graphs_except_one,
            atom_tools_utils.is_document_open(material_graph_document_ids[0]) is True and
            atom_tools_utils.is_document_open(material_graph_document_ids[1]) is False and
            atom_tools_utils.is_document_open(material_graph_document_ids[2]) is False and
            atom_tools_utils.is_document_open(material_graph_document_ids[3]) is False and
            atom_tools_utils.is_document_open(material_graph_document_ids[4]) is False)

        # 6. Verify Node Palette pane visibility.
        atom_tools_utils.set_pane_visibility("Node Palette", True)
        Report.result(
            Tests.node_palette_pane_visible,
            atom_tools_utils.is_pane_visible("Node Palette") is True)

        # 7. Verify material graph name is 'test1'.
        material_graph_name = material_canvas_utils.get_graph_name(material_graph_document_ids[0])
        Report.result(
            Tests.material_graph_name_is_test1,
            material_graph_name == "test1")

        # 8. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(MaterialCanvas_BasicFunctionalityChecks_AllChecksPass)
