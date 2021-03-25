"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C1564080: Help Menu Function
https://testrail.agscollab.com/index.php?/cases/view/1564080
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 100


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
class TestHelpMenuFunction(object):
    @pytest.fixture(autouse=True)
    def setup_teardown(self, request, workspace, editor, project):
        def teardown():
            workspace = editor.workspace

        # Setup - add the teardown finalizer
        request.addfinalizer(teardown)

    expected_lines = [
        "Getting Started Action triggered Url https://docs.aws.amazon.com/lumberyard/latest/gettingstartedguide",
        "Tutorials Action triggered Url https://www.youtube.com/amazonlumberyardtutorials",
        "Glossary Action triggered Url https://docs.aws.amazon.com/lumberyard/userguide/glossary",
        "Lumberyard Documentation Action triggered Url https://docs.aws.amazon.com/lumberyard/userguide",
        "GameLift Documentation Action triggered Url https://docs.aws.amazon.com/gamelift/developerguide",
        "Release Notes Action triggered Url https://docs.aws.amazon.com/lumberyard/releasenotes",
        "GameDev Blog Action triggered Url https://aws.amazon.com/blogs/gamedev",
        "GameDev Twitch Channel Action triggered Url http://twitch.tv/amazongamedev",
        "Forums Action triggered Url https://gamedev.amazon.com/forums",
        "AWS Support Action triggered Url https://aws.amazon.com/contact-us",
        "Documentation search with 'component' text triggered Url https://docs.aws.amazon.com/search/doc-search.html?searchPath=documentation-product&searchQuery=component&this_doc_product=Amazon Lumberyard&ref=lye&ev=0.0.0.0#facet_doc_product=Amazon Lumberyard",
        "Documentation Search with '' text triggered Url https://aws.amazon.com/documentation/lumberyard/",
    ]
    
    @pytest.mark.test_case_id("C1564080")
    def test_help_menu_function(self, request, editor):
        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "help_menu_function.py",
            self.expected_lines,
            cfg_args=[],
            timeout=log_monitor_timeout,
        )
