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
    world_position_node_created = (
        "New AZStd::shared_ptr<Node> object created in memory as world_position_node",
        "P0: Failed to create a new world_position_node in memory.")
    standard_pbr_node_created = (
        "New AZStd::shared_ptr<Node> object created in memory as standard_pbr_node.",
        "P0: Failed to create a new standard_pbr_node in memory.")
    nodes_connected = (
        "Connected world_position_node to standard_pbr_node successfully.",
        "P0: Failed to connect world_position_node to standard_pbr_node.")


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
    5) Use the CloseAllDocumentsExcept bus call to close all but test_1_material_graph.
    6) Verify Node Palette pane visibility.
    7) Verify test_1_material_graph.name is 'test1'.
    8) Create a new world_position_node inside test_1_material_graph.
    9) Create a new standard_pbr_node inside test_1_material_graph.
    10) Create a node connection between world_position_node outbound slot and standard_pbr_node inbound slot.
    11) Look for errors and asserts.

    :return: None
    """

    import os

    import azlmbr.math as math

    import Atom.atom_utils.atom_tools_utils as atom_tools_utils
    from Atom.atom_utils.material_canvas_utils import Graph, Node
    from editor_python_test_tools.utils import Report, Tracer, TestHelper

    with Tracer() as error_tracer:
        # Disable registry settings to prevent message boxes from blocking test progression.
        atom_tools_utils.disable_document_message_box_settings()

        # Disable automatic material and shader generation when opening graphs (to avoid any file writing/saving).
        atom_tools_utils.disable_graph_compiler_settings()

        # Set constants before starting test steps.
        test_1_material_graph_file = os.path.join(atom_tools_utils.MATERIALCANVAS_GRAPH_PATH, "test1.materialgraph")
        test_2_material_graph_file = os.path.join(atom_tools_utils.MATERIALCANVAS_GRAPH_PATH, "test2.materialgraph")
        test_3_material_graph_file = os.path.join(atom_tools_utils.MATERIALCANVAS_GRAPH_PATH, "test3.materialgraph")
        test_4_material_graph_file = os.path.join(atom_tools_utils.MATERIALCANVAS_GRAPH_PATH, "test4.materialgraph")
        test_5_material_graph_file = os.path.join(atom_tools_utils.MATERIALCANVAS_GRAPH_PATH, "test5.materialgraph")

        # 1. Open an existing material graph document.
        test_1_material_graph = Graph(test_1_material_graph_file)
        Report.result(
            Tests.open_existing_material_graph,
            atom_tools_utils.is_document_open(test_1_material_graph.document_id) is True)

        # 2. Close the selected material graph document.
        atom_tools_utils.close_document(test_1_material_graph.document_id)
        Report.result(
            Tests.close_opened_material_graph,
            atom_tools_utils.is_document_open(test_1_material_graph.document_id) is False)

        # 3. Open multiple material graph documents then use the CloseAllDocuments bus call to close them all.
        material_graph_document_files = [
            test_1_material_graph_file, test_2_material_graph_file, test_3_material_graph_file,
            test_4_material_graph_file, test_5_material_graph_file]
        for material_graph_document_file in material_graph_document_files:
            Graph(material_graph_document_file)
        Report.result(
            Tests.close_all_opened_material_graphs,
            atom_tools_utils.close_all_documents() is True)

        # 4. Open multiple material graph documents then verify all material documents are opened.
        test_material_graphs = []
        for material_graph_document_file in material_graph_document_files:
            test_material_graph = Graph(material_graph_document_file)
            Report.result(Tests.verify_all_material_graphs_are_opened,
                          atom_tools_utils.is_document_open(test_material_graph.document_id) is True)
            test_material_graphs.append(test_material_graph.document_id)

        # 5. Use the CloseAllDocumentsExcept bus call to close all but test_1_material_graph.
        test_1_material_graph = test_material_graphs[0]
        atom_tools_utils.close_all_except_selected(test_1_material_graph.document_id)
        Report.result(
            Tests.close_all_opened_material_graphs_except_one,
            atom_tools_utils.is_document_open(test_1_material_graph) is True and
            atom_tools_utils.is_document_open(test_material_graphs[1]) is False and
            atom_tools_utils.is_document_open(test_material_graphs[2]) is False and
            atom_tools_utils.is_document_open(test_material_graphs[3]) is False and
            atom_tools_utils.is_document_open(test_material_graphs[4]) is False)

        # 6. Verify Node Palette pane visibility.
        atom_tools_utils.set_pane_visibility("Node Palette", True)
        Report.result(
            Tests.node_palette_pane_visible,
            atom_tools_utils.is_pane_visible("Node Palette") is True)

        # 7. Verify test_1_material_graph.name is 'test1'.
        Report.result(
            Tests.material_graph_name_is_test1,
            test_1_material_graph.name == "test1")

        # 8. Create a new world_position_node inside test_1_material_graph.
        test_1_material_graph.add_node('World Position', math.Vector2(-200.0, 10.0))
        world_position_node = test_1_material_graph.nodes[0]
        Report.result(
            Tests.world_position_node_created,
            world_position_node.node_object.typename == "AZStd::shared_ptr<Node>")

        # 9. Create a new standard_pbr_node inside test_1_material_graph.
        test_1_material_graph.add_node('Standard PBR', math.Vector2(10.0, 220.0))
        standard_pbr_node = test_1_material_graph.nodes[1]
        Report.result(
            Tests.standard_pbr_node_created,
            standard_pbr_node.node_object.typename == "AZStd::shared_ptr<Node>")

        # 10. Create a node connection between world_position_node outbound slot and standard_pbr_node inbound slot.
        world_position_node.connect_to_node_inbound_slot(standard_pbr_node)
        are_slots_connected = world_position_node.is_connected_to_node_inbound_slot(standard_pbr_node)
        Report.result(Tests.nodes_connected, are_slots_connected is True)

        # 11. Look for errors and asserts.
        TestHelper.wait_for_condition(lambda: error_tracer.has_errors or error_tracer.has_asserts, 1.0)
        for error_info in error_tracer.errors:
            Report.info(f"Error: {error_info.filename} {error_info.function} | {error_info.message}")
        for assert_info in error_tracer.asserts:
            Report.info(f"Assert: {assert_info.filename} {assert_info.function} | {assert_info.message}")


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(MaterialCanvas_BasicFunctionalityChecks_AllChecksPass)
