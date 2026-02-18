"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Processor GUI Tests
"""

# Import builtin libraries
import pytest
import logging
import os
import tempfile as TF

# Import LyTestTools
import ly_test_tools
import ly_test_tools.builtin.helpers as helpers
import ly_test_tools.environment.waiter as waiter
import ly_test_tools.environment.file_system as fs
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.launchers.launcher_helper as launcher_helper
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP
from ly_test_tools.o3de.asset_processor import StopReason

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture
from ..ap_fixtures.ap_idle_fixture import TimestampChecker
from ..ap_fixtures.ap_fast_scan_setting_backup_fixture import (
    ap_fast_scan_setting_backup_fixture as fast_scan_backup,
)

# Use the following logging pattern to hook all test logging together:
logger = logging.getLogger(__name__)
# Configuring the logging is done in ly_test_tools at the following location:
# ~/dev/Tools/LyTestTools/ly_test_tools/log/py_logging_util.py

# Helper: variables we will use for parameter values in the test:
targetProjects = ["AutomatedTesting"]

@pytest.fixture
def ap_idle(workspace, ap_setup_fixture):
    gui_timeout_max = 30
    return TimestampChecker(workspace.paths.asset_catalog(ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]), gui_timeout_max)

def setup_temp_dir() -> str:
    tempDir = os.path.join(TF.gettempdir(), "TempTest")
    if os.path.exists(tempDir):
        fs.delete([tempDir], True, True)
    if not os.path.exists(tempDir):
        os.mkdir(tempDir)
    return tempDir

@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.SUITE_main
class TestsAssetProcessorGUI(object):
    """
    Specific Tests for Asset Processor GUI
    """

    def test_GenericAssetBuilder_OutputsDependencies(self, asset_processor, ap_setup_fixture, workspace):

        """
        Ensures the Generic Asset Builder actually picks up the dependencies of an asset automatically.
        Test Steps:
        1. Start Asset Processor without quitonidle
        2. Execute AssetBuilder with a command line that makes it dump the actual job output
           into a folder instead of the cache.
        3. Inspect the job output and verify everything is as expected.
        """
        env = ap_setup_fixture

        # Copy test assets to project folder and verify test assets folder exists

        result, _ = asset_processor.gui_process(quitonidle=False,connect_to_ap=True)
        assert result, "AP GUI failed"

        # Wait for AP to have finished processing
        asset_processor.wait_for_idle()

        path_to_test_asset = os.path.join(workspace.paths.engine_root(), "Gems", "TestAssetBuilder", "Assets", "GenericAssetBuilderAssets", "asset_with_dependencies.auto_test_depasset")
        path_to_output_dir = setup_temp_dir()
        result, _ = asset_processor.run_builder(extra_params=
                 ["-debug", path_to_test_asset,
                  "-output", path_to_output_dir])
        assert result, "AP Builder failed"

        # examine results
        output_processjobs_xml = os.path.join(path_to_output_dir, "Generic Asset Builder", "ProcessJobs", "0_ProcessJobResponse.xml")

        import xml.etree.ElementTree as ET
        tree = ET.parse(output_processjobs_xml)
        root = tree.getroot()

        # The entire output XML file looks something like this.
        # set of "class" structures whihc looks something like this:
        '''
        <ObjectStream version="3">
            <Class name="ProcessJobResponse" version="3"">
                <Class name="AZStd::vector&lt;JobProduct, allocator&gt;" field="Output Products"">
                    <Class name="JobProduct" field="element" version="7">
                        <Class name="AZStd::string" field="Product File Name"/>
                        <Class name="AZ::Uuid" field="Product Asset Type"/>
                        <Class name="unsigned int" field="Product Sub Id" value="0"/>
                        <Class name="AZStd::vector&lt;unsigned int, allocator&gt;" field="Legacy Sub Ids"/>
                        <Class name="AZStd::vector&lt;ProductDependency, allocator&gt;" field="Dependencies">
                            ( Repeated set of product dependencies here, see below)
        '''
       
        # here is an example of a ProductDependency section in the output XML:
        # it consists of an "AssetId" which is a pair of (guid, subid) and a set of flags.
        '''
        <Class name="ProductDependency" field="element" version="1">
            <Class name="AssetId" field="Dependency Id" version="1">
                <Class name="AZ::Uuid" field="guid" value="{4CACAE2C-C361-57BE-A8D5-647F07660A3F}"/>
                <Class name="unsigned int" field="subId" value="0"/>
            </Class>
            <Class name="AZStd::bitset&lt;64&gt;" field="Flags" value="0200000000000000"/>
        </Class>
        '''

        # given a treeElement that points at the tree element with <Class name="ProductDependency"> as above, extract the guid and flags.
        def extractProductDependencyInfo(treeElement):
            guid = None
            flags = None
            foundFields = 0
            for elem in treeElement.iter():
                if elem.tag == "Class" and elem.attrib.get("field") == "guid":
                    guid = elem.attrib.get("value")
                    foundFields += 1
                elif elem.tag == "Class" and elem.attrib.get("field") == "Flags":
                    flags = elem.attrib.get("value")
                    foundFields += 1
                if foundFields == 2:
                    break
            assert (foundFields == 2), f"Expected to find both guid and flags for a ProductDependency, but only found {foundFields} fields."
            return guid, flags
    
        found_dependencies = []
    
        # find the dependencies section, by iterating over the tree until we find an object of type "Class"
        # with a name field of "ProductDependency":
        for elem in root.iter():
            if elem.tag == "Class" and elem.attrib.get("name") == "ProductDependency":
                # we found a ProductDependency.  It will have 2 child nodes, the first of which
                # has 2 children of its own, "Class" node with "Uuid" being one of the uuids.
                # The second child node will have a "Class" node with a "AZStd::bitset&lt;64&gt;" field that we care about.
                guid, flags = extractProductDependencyInfo(elem)
                found_dependencies.append((guid, flags))

        # we expect to find 3 dependencies, two from the asset<t> fields, and one from the loose assetid field.
        num_expected = 3
        num_found = 0
        for found_dependency in found_dependencies:
            guid, flags = found_dependency
            if guid == "{4CACAE2C-C361-57BE-A8D5-647F07660A3F}":
                num_found += 1
                assert flags == "0000000000000000", f"Flags for dependency with guid {guid} were expected to be 0000000000000000 but were {flags}"
            elif guid == "{6677FA47-391A-591A-BC9E-279A03F75974}":
                num_found += 1
                assert flags == "0200000000000000", f"Flags for dependency with guid {guid} were expected to be 0200000000000000 but were {flags}"
            elif guid == "{4D07F6E7-3D4E-59FA-840D-2757B523650C}":
                num_found += 1
                assert flags == "0200000000000000", f"Flags for dependency with guid {guid} were expected to be 0200000000000000 but were {flags}"
            else:
                pytest.fail(f"Found a dependency with guid {guid} which was not expected.")
        
        assert num_found == num_expected, f"Expected to find {num_expected} dependencies but found {num_found}."

