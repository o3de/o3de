#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
import json
import pathlib
import uuid

import pytest
import string

from o3de import engine_template
from unittest.mock import patch

CPP_LICENSE_TEXT = """// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}
"""


TEST_TEMPLATED_CONTENT_WITHOUT_LICENSE = """
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}Requests
    {
    public:
        AZ_RTTI(${SanitizedCppName}Requests, "{${Random_Uuid}}");
        virtual ~${SanitizedCppName}Requests() = default;
        // Put your public methods here
    };

    class ${SanitizedCppName}BusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using ${SanitizedCppName}RequestBus = AZ::EBus<${SanitizedCppName}Requests, ${SanitizedCppName}BusTraits>;
    using ${SanitizedCppName}Interface = AZ::Interface<${SanitizedCppName}Requests>;

} // namespace ${SanitizedCppName}
"""

TEST_TEMPLATED_CONTENT_WITH_LICENSE = CPP_LICENSE_TEXT + TEST_TEMPLATED_CONTENT_WITHOUT_LICENSE

TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITHOUT_LICENSE = string.Template(
    TEST_TEMPLATED_CONTENT_WITHOUT_LICENSE).safe_substitute({'SanitizedCppName': "TestTemplate"})

TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITH_LICENSE = string.Template(
    TEST_TEMPLATED_CONTENT_WITH_LICENSE).safe_substitute({'SanitizedCppName': "TestTemplate"})

TEST_CONCRETE_TESTPROJECT_TEMPLATE_CONTENT_WITHOUT_LICENSE = string.Template(
    TEST_TEMPLATED_CONTENT_WITHOUT_LICENSE).safe_substitute({'SanitizedCppName': "TestProject"})

TEST_CONCRETE_TESTPROJECT_TEMPLATE_CONTENT_WITH_LICENSE = string.Template(
    TEST_TEMPLATED_CONTENT_WITH_LICENSE).safe_substitute({'SanitizedCppName': "TestProject"})

TEST_CONCRETE_TESTGEM_TEMPLATE_CONTENT_WITHOUT_LICENSE = string.Template(
    TEST_TEMPLATED_CONTENT_WITHOUT_LICENSE).safe_substitute({'SanitizedCppName': "TestGem"})

TEST_CONCRETE_TESTGEM_TEMPLATE_CONTENT_WITH_LICENSE = string.Template(
    TEST_TEMPLATED_CONTENT_WITH_LICENSE).safe_substitute({'SanitizedCppName': "TestGem"})

TEST_TEMPLATE_JSON_CONTENTS = """\
{
    "template_name": "Templates",
    "origin": "The primary repo for Templates goes here: i.e. http://www.mydomain.com",
    "license": "What license Templates uses goes here: i.e. https://opensource.org/licenses/MIT",
    "display_name": "Templates",
    "summary": "A short description of Templates.",
    "canonical_tags": [],
    "user_tags": [
        "Templates"
    ],
    "icon_path": "preview.png",
    "copyFiles": [
        {
            "file": "Code/Include/${Name}/${Name}Bus.h",
            "origin": "Code/Include/${Name}/${Name}Bus.h",
            "isTemplated": true,
            "isOptional": false
        },
        {
            "file": "Code/Include/Platform/Salem/${Name}Bus.h",
            "origin": "Code/Include/Platform/Salem/${Name}Bus.h",
            "isTemplated": true,
            "isOptional": false
        }
    ],
    "createDirectories": [
        {
            "dir": "Code",
            "origin": "Code"
        },
        {
            "dir": "Code/Include",
            "origin": "Code/Include"
        },
        {
            "dir": "Code/Include/${Name}",
            "origin": "Code/Include/${Name}"
        },
        {
            "dir": "Code/Include/Platform",
            "origin": "Code/Include/Platform"
        },
        {
            "dir": "Code/Include/Platform/Salem",
            "origin": "Code/Include/Platform/Salem"
        }
    ]
}
"""


TEST_CONCRETE_TEMPLATE_JSON_CONTENTS = string.Template(
    TEST_TEMPLATE_JSON_CONTENTS).safe_substitute({'Name': 'TestTemplate'})


TEST_CONCRETE_PROJECT_TEMPLATE_JSON_CONTENTS = string.Template(
    TEST_TEMPLATE_JSON_CONTENTS).safe_substitute({'Name': 'TestProject'})


TEST_CONCRETE_GEM_TEMPLATE_JSON_CONTENTS = string.Template(
    TEST_TEMPLATE_JSON_CONTENTS).safe_substitute({'Name': 'TestGem'})


@pytest.mark.parametrize(
    "concrete_contents,"
    " templated_contents_with_license, templated_contents_without_license,"
    " keep_license_text, force, expect_failure,"
    " template_json_contents", [
        pytest.param(TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITH_LICENSE,
                     TEST_TEMPLATED_CONTENT_WITH_LICENSE, TEST_TEMPLATED_CONTENT_WITHOUT_LICENSE,
                     True, True, False,
                     TEST_TEMPLATE_JSON_CONTENTS),
        pytest.param(TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITH_LICENSE,
                     TEST_TEMPLATED_CONTENT_WITH_LICENSE, TEST_TEMPLATED_CONTENT_WITHOUT_LICENSE,
                     False, True, False,
                     TEST_TEMPLATE_JSON_CONTENTS)
    ]
)
def test_create_template(tmpdir,
                         concrete_contents,
                         templated_contents_with_license, templated_contents_without_license,
                         keep_license_text, force, expect_failure,
                         template_json_contents):
    engine_root = (pathlib.Path(tmpdir) / 'engine-root').resolve()
    engine_root.mkdir(parents=True, exist_ok=True)

    template_source_path = engine_root / 'TestTemplates'

    engine_gem_code_include_testgem = template_source_path / 'Code/Include/TestTemplate'
    engine_gem_code_include_testgem.mkdir(parents=True, exist_ok=True)

    gem_bus_file = engine_gem_code_include_testgem / 'TestTemplateBus.h'
    with gem_bus_file.open('w') as s:
        s.write(concrete_contents)

    engine_gem_code_include_platform_salem = template_source_path / 'Code/Include/Platform/Salem'
    engine_gem_code_include_platform_salem.mkdir(parents=True, exist_ok=True)

    restricted_gem_bus_file = engine_gem_code_include_platform_salem / 'TestTemplateBus.h'
    with restricted_gem_bus_file.open('w') as s:
        s.write(concrete_contents)

    template_folder =  engine_root / 'Templates'
    template_folder.mkdir(parents=True, exist_ok=True)

    result = engine_template.create_template(template_source_path, template_folder, source_name='TestTemplate',
                                             keep_license_text=keep_license_text, force=force)

    if expect_failure:
        assert result != 0
    else:
        assert result == 0
        new_template_folder = template_folder
        assert new_template_folder.is_dir()
        new_template_json = new_template_folder / 'template.json'
        assert new_template_json.is_file()
        with new_template_json.open('r') as s:
            s_data = s.read()
        assert json.loads(s_data) == json.loads(template_json_contents)

        template_content_folder = new_template_folder / 'Template'
        new_default_name_bus_file = template_content_folder / 'Code/Include/${Name}/${Name}Bus.h'
        assert new_default_name_bus_file.is_file()
        with new_default_name_bus_file.open('r') as s:
            s_data = s.read()
        if keep_license_text:
            assert s_data == templated_contents_with_license
        else:
            assert s_data == templated_contents_without_license

        platform_template_folder = engine_root / 'Salem/Templates'

        new_platform_default_name_bus_file = template_content_folder / 'Code/Include/Platform/Salem/${Name}Bus.h'
        assert new_platform_default_name_bus_file.is_file()
        with new_platform_default_name_bus_file.open('r') as s:
            s_data = s.read()
        if keep_license_text:
            assert s_data == templated_contents_with_license
        else:
            assert s_data == templated_contents_without_license


class TestCreateTemplate:
    def instantiate_template_wrapper(self, tmpdir, create_from_template_func, instantiated_name,
                                     concrete_contents, templated_contents,
                                     keep_license_text, force, expect_failure,
                                     template_json_contents, template_file_creation_map = {},
                                     **create_from_template_kwargs):
        # Use a SHA-1 Hash of the destination_name for every Random_Uuid for determinism in the test
        concrete_contents = string.Template(concrete_contents).safe_substitute(
            {'Random_Uuid': uuid.uuid5(uuid.NAMESPACE_DNS, instantiated_name)})

        engine_root = (pathlib.Path(tmpdir) / 'engine-root').resolve()
        engine_root.mkdir(parents=True, exist_ok=True)

        template_default_folder = engine_root / 'Templates/Default'
        template_default_folder.mkdir(parents=True, exist_ok=True)

        template_json = template_default_folder / 'template.json'
        with template_json.open('w') as s:
            s.write(template_json_contents)

        for file_template_filename, file_template_content in template_file_creation_map.items():
            file_template_path = template_default_folder / 'Template' / file_template_filename
            file_template_path.parent.mkdir(parents=True, exist_ok=True)
            with file_template_path.open('w') as file_template_handle:
                file_template_handle.write(file_template_content)

        default_name_bus_dir = template_default_folder / 'Template/Code/Include/${Name}'
        default_name_bus_dir.mkdir(parents=True, exist_ok=True)

        default_name_bus_file = default_name_bus_dir / '${Name}Bus.h'
        with default_name_bus_file.open('w') as s:
            s.write(templated_contents)

        template_content_folder = template_default_folder / 'Template'
        platform_default_name_bus_dir = template_content_folder / 'Code/Include/Platform/Salem'
        platform_default_name_bus_dir.mkdir(parents=True, exist_ok=True)

        platform_default_name_bus_file = platform_default_name_bus_dir / '${Name}Bus.h'
        with platform_default_name_bus_file.open('w') as s:
            s.write(templated_contents)

        template_dest_path = engine_root / instantiated_name
        # Skip registeration in test
        with patch('uuid.uuid4', return_value=uuid.uuid5(uuid.NAMESPACE_DNS, instantiated_name)) as uuid4_mock:
            result = create_from_template_func(template_dest_path, template_path=template_default_folder, force=True,
                                                          keep_license_text=keep_license_text, **create_from_template_kwargs)
        if expect_failure:
            assert result != 0
        else:
            assert result == 0

            test_folder = template_dest_path
            assert test_folder.is_dir()

            test_bus_file = test_folder / f'Code/Include/{instantiated_name}/{instantiated_name}Bus.h'
            assert test_bus_file.is_file()
            with test_bus_file.open('r') as s:
                s_data = s.read()
            assert s_data == concrete_contents

            platform_test_bus_folder = test_folder / 'Code/Include/Platform/Salem'
            assert platform_test_bus_folder.is_dir()

            platform_default_name_bus_file = platform_test_bus_folder / f'{instantiated_name}Bus.h'
            assert platform_default_name_bus_file.is_file()
            with platform_default_name_bus_file.open('r') as s:
                s_data = s.read()
            assert s_data == concrete_contents


    # Use a SHA-1 Hash of the destination_name for every Random_Uuid for determinism in the test
    @pytest.mark.parametrize(
        "concrete_contents, templated_contents,"
        " keep_license_text, force, expect_failure,"
        " template_json_contents", [
            pytest.param(TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITH_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                         True, True, False,
                         TEST_TEMPLATE_JSON_CONTENTS),
            pytest.param(TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITHOUT_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                         False, True, False,
                         TEST_TEMPLATE_JSON_CONTENTS)
        ]
    )
    def test_create_from_template(self, tmpdir, concrete_contents, templated_contents, keep_license_text, force,
                                  expect_failure, template_json_contents):

        self.instantiate_template_wrapper(tmpdir, engine_template.create_from_template, 'TestTemplate', concrete_contents,
                                          templated_contents, keep_license_text, force, expect_failure,
                                          template_json_contents, destination_name='TestTemplate')


    @pytest.mark.parametrize(
        "concrete_contents, templated_contents,"
        " keep_license_text, force, expect_failure,"
        " template_json_contents", [
            pytest.param(TEST_CONCRETE_TESTPROJECT_TEMPLATE_CONTENT_WITH_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                         True, True, False,
                         TEST_TEMPLATE_JSON_CONTENTS),
            pytest.param(TEST_CONCRETE_TESTPROJECT_TEMPLATE_CONTENT_WITHOUT_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                         False, True, False,
                         TEST_TEMPLATE_JSON_CONTENTS)
        ]
    )
    def test_create_project(self, tmpdir, concrete_contents, templated_contents, keep_license_text, force,
                                  expect_failure, template_json_contents):
        template_file_map = { 'project.json':
                              '''
                              {
                               "project_name": "${Name}"
                              }
                              '''}

        # Append the project.json to the list of files to copy from the template
        template_json_dict = json.loads(template_json_contents)
        template_json_dict.setdefault('copyFiles', []).append(
        {
            "file": "project.json",
            "origin": "project.json",
            "isTemplated": True,
            "isOptional": False
        })
        # Convert the python dictionary back into a json string
        template_json_contents = json.dumps(template_json_dict, indent=4)
        self.instantiate_template_wrapper(tmpdir, engine_template.create_project, 'TestProject', concrete_contents,
                                          templated_contents, keep_license_text, force, expect_failure,
                                          template_json_contents, template_file_map, project_name='TestProject', no_register=True)


    @pytest.mark.parametrize(
        "concrete_contents, templated_contents,"
        " keep_license_text, force, expect_failure,"
        " template_json_contents", [
            pytest.param(TEST_CONCRETE_TESTGEM_TEMPLATE_CONTENT_WITH_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                         True, True, False,
                         TEST_TEMPLATE_JSON_CONTENTS),
            pytest.param(TEST_CONCRETE_TESTGEM_TEMPLATE_CONTENT_WITHOUT_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                         False, True, False,
                         TEST_TEMPLATE_JSON_CONTENTS)
        ]
    )
    def test_create_gem(self, tmpdir, concrete_contents, templated_contents, keep_license_text, force,
                                  expect_failure, template_json_contents):
        # Create a gem.json file in the template folder
        template_file_map = {'gem.json':
                                 '''
                                  {
                                   "gem_name": "${Name}"
                                  }
                                  '''}

        # Append the gem.json to the list of files to copy from the template
        template_json_dict = json.loads(template_json_contents)
        template_json_dict.setdefault('copyFiles', []).append(
            {
                "file": "gem.json",
                "origin": "gem.json",
                "isTemplated": True,
                "isOptional": False
            })
        #Convert dict back to string
        template_json_contents = json.dumps(template_json_dict, indent=4)
        self.instantiate_template_wrapper(tmpdir, engine_template.create_gem, 'TestGem', concrete_contents,
                                          templated_contents, keep_license_text, force, expect_failure,
                                          template_json_contents, template_file_map, gem_name='TestGem', no_register=True)
