#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
import os
import pytest
from . import engine_template

TEST_TEMPLATED_CONTENT_WITH_LICENSE = """\
// {BEGIN_LICENSE}
/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
// {END_LICENSE}
#pragma once

#include <AzCore/EBus/EBus.h>

namespace ${Name}
{
    class ${Name}Requests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
    };

    using ${Name}RequestsBus = AZ::EBus<${Name}Requests>;

} // namespace ${Name}

"""

TEST_TEMPLATED_CONTENT_WITHOUT_LICENSE = """\
#pragma once

#include <AzCore/EBus/EBus.h>

namespace ${Name}
{
    class ${Name}Requests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
    };

    using ${Name}RequestsBus = AZ::EBus<${Name}Requests>;

} // namespace ${Name}

"""

TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITHOUT_LICENSE = """\
#pragma once

#include <AzCore/EBus/EBus.h>

namespace TestTemplate
{
    class TestTemplateRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
    };

    using TestTemplateRequestsBus = AZ::EBus<TestTemplateRequests>;

} // namespace TestTemplate

"""

TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITH_LICENSE = """\
// {BEGIN_LICENSE}
/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
// {END_LICENSE}
#pragma once

#include <AzCore/EBus/EBus.h>

namespace TestTemplate
{
    class TestTemplateRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
    };

    using TestTemplateRequestsBus = AZ::EBus<TestTemplateRequests>;

} // namespace TestTemplate

"""

TEST_CONCRETE_TESTPROJECT_TEMPLATE_CONTENT_WITHOUT_LICENSE = """\
#pragma once

#include <AzCore/EBus/EBus.h>

namespace TestProject
{
    class TestProjectRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
    };

    using TestProjectRequestsBus = AZ::EBus<TestProjectRequests>;

} // namespace TestProject

"""

TEST_CONCRETE_TESTPROJECT_TEMPLATE_CONTENT_WITH_LICENSE = """\
// {BEGIN_LICENSE}
/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
// {END_LICENSE}
#pragma once

#include <AzCore/EBus/EBus.h>

namespace TestProject
{
    class TestProjectRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
    };

    using TestProjectRequestsBus = AZ::EBus<TestProjectRequests>;

} // namespace TestProject

"""

TEST_CONCRETE_TESTGEM_TEMPLATE_CONTENT_WITHOUT_LICENSE = """\
#pragma once

#include <AzCore/EBus/EBus.h>

namespace TestGem
{
    class TestGemRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
    };

    using TestGemRequestsBus = AZ::EBus<TestGemRequests>;

} // namespace TestGem

"""

TEST_CONCRETE_TESTGEM_TEMPLATE_CONTENT_WITH_LICENSE = """\
// {BEGIN_LICENSE}
/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
// {END_LICENSE}
#pragma once

#include <AzCore/EBus/EBus.h>

namespace TestGem
{
    class TestGemRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        // Put your public methods here
    };

    using TestGemRequestsBus = AZ::EBus<TestGemRequests>;

} // namespace TestGem

"""

TEST_DEFAULTTEMPLATE_JSON_CONTENTS = """\
{
    "inputPath": "Templates/Default/Template",
    "copyFiles": [
        {
            "inFile": "Code/Include/${Name}/${Name}Bus.h",
            "outFile": "Code/Include/${Name}/${Name}Bus.h",
            "isTemplated": true,
            "isOptional": false
        }
    ],
    "createDirectories": [
        {
            "outDir": "Code"
        },
        {
            "outDir": "Code/Include"
        },
        {
            "outDir": "Code/Include/Platform"
        },
        {
            "outDir": "Code/Include/${Name}"
        }
    ]
}\
"""

TEST_DEFAULTTEMPLATE_RESTRICTED_JSON_CONTENTS = """\
{
    "inputPath": "restricted/Salem/Templates/Default/Template",
    "copyFiles": [
        {
            "inFile": "Code/Include/Platform/Salem/${Name}Bus.h",
            "outFile": "Code/Include/Platform/Salem/${Name}Bus.h",
            "isTemplated": true,
            "isOptional": false
        }
    ],
    "createDirectories": [
        {
            "outDir": "Code/Include/Platform/Salem"
        }
    ]
}\
"""

TEST_DEFAULTPROJECT_TEMPLATE_JSON_CONTENTS = """\
{
    "inputPath": "Templates/DefaultProject/Template",
    "copyFiles": [
        {
            "inFile": "Code/Include/${Name}/${Name}Bus.h",
            "outFile": "Code/Include/${Name}/${Name}Bus.h",
            "isTemplated": true,
            "isOptional": false
        }
    ],
    "createDirectories": [
        {
            "outDir": "Code"
        },
        {
            "outDir": "Code/Include"
        },
        {
            "outDir": "Code/Include/Platform"
        },
        {
            "outDir": "Code/Include/${Name}"
        }
    ]
}\
"""

TEST_DEFAULTPROJECT_TEMPLATE_RESTRICTED_JSON_CONTENTS = """\
{
    "inputPath": "restricted/Salem/Templates/DefaultProject/Template",
    "copyFiles": [
        {
            "inFile": "Code/Include/Platform/Salem/${Name}Bus.h",
            "outFile": "Code/Include/Platform/Salem/${Name}Bus.h",
            "isTemplated": true,
            "isOptional": false
        }
    ],
    "createDirectories": [
        {
            "outDir": "Code/Include/Platform/Salem"
        }
    ]
}\
"""

TEST_DEFAULTGEM_TEMPLATE_JSON_CONTENTS = """\
{
    "inputPath": "Templates/DefaultGem/Template",
    "copyFiles": [
        {
            "inFile": "Code/Include/${Name}/${Name}Bus.h",
            "outFile": "Code/Include/${Name}/${Name}Bus.h",
            "isTemplated": true,
            "isOptional": false
        }
    ],
    "createDirectories": [
        {
            "outDir": "Code"
        },
        {
            "outDir": "Code/Include"
        },
        {
            "outDir": "Code/Include/Platform"
        },
        {
            "outDir": "Code/Include/${Name}"
        }
    ]
}\
"""

TEST_DEFAULTGEM_TEMPLATE_RESTRICTED_JSON_CONTENTS = """\
{
    "inputPath": "restricted/Salem/Templates/DefaultGem/Template",
    "copyFiles": [
        {
            "inFile": "Code/Include/Platform/Salem/${Name}Bus.h",
            "outFile": "Code/Include/Platform/Salem/${Name}Bus.h",
            "isTemplated": true,
            "isOptional": false
        }
    ],
    "createDirectories": [
        {
            "outDir": "Code/Include/Platform/Salem"
        }
    ]
}\
"""


@pytest.mark.parametrize(
    "concrete_contents,"
    " templated_contents_with_license, templated_contents_without_license,"
    " keep_license_text, expect_failure,"
    " template_json_contents, restricted_template_json_contents", [
        pytest.param(TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITH_LICENSE,
                     TEST_TEMPLATED_CONTENT_WITH_LICENSE, TEST_TEMPLATED_CONTENT_WITHOUT_LICENSE,
                     True, False,
                     TEST_DEFAULTTEMPLATE_JSON_CONTENTS, TEST_DEFAULTTEMPLATE_RESTRICTED_JSON_CONTENTS),
        pytest.param(TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITH_LICENSE,
                     TEST_TEMPLATED_CONTENT_WITH_LICENSE, TEST_TEMPLATED_CONTENT_WITHOUT_LICENSE,
                     False, False,
                     TEST_DEFAULTTEMPLATE_JSON_CONTENTS, TEST_DEFAULTTEMPLATE_RESTRICTED_JSON_CONTENTS)
    ]
)
def test_create_template(tmpdir,
                         concrete_contents,
                         templated_contents_with_license, templated_contents_without_license,
                         keep_license_text, expect_failure,
                         template_json_contents, restricted_template_json_contents):
    dev_root = str(tmpdir.join('dev').realpath()).replace('\\', '/')
    os.makedirs(dev_root, exist_ok=True)

    dev_gem_code_include_testgem = f'{dev_root}/TestTemplate/Code/Include/TestTemplate'
    os.makedirs(dev_gem_code_include_testgem, exist_ok=True)

    gem_bus_file = f'{dev_gem_code_include_testgem}/TestTemplateBus.h'
    if os.path.isfile(gem_bus_file):
        os.unlink(gem_bus_file)
    with open(gem_bus_file, 'w') as s:
        s.write(concrete_contents)

    dev_gem_code_include_platform_salem = f'{dev_root}/TestTemplate/Code/Include/Platform/Salem'
    os.makedirs(dev_gem_code_include_platform_salem, exist_ok=True)

    restricted_gem_bus_file = f'{dev_gem_code_include_platform_salem}/TestTemplateBus.h'
    if os.path.isfile(restricted_gem_bus_file):
        os.unlink(restricted_gem_bus_file)
    with open(restricted_gem_bus_file, 'w') as s:
        s.write(concrete_contents)

    template_folder = f'{dev_root}/Templates'
    os.makedirs(template_folder, exist_ok=True)

    restricted_folder = f'{dev_root}/restricted'
    os.makedirs(restricted_folder, exist_ok=True)

    result = engine_template.create_template(dev_root, 'TestTemplate', 'Default', keep_license_text=keep_license_text)

    if expect_failure:
        assert result != 0
    else:
        assert result == 0
        new_template_folder = f'{template_folder}/Default'
        assert os.path.isdir(new_template_folder)
        new_template_json = f'{new_template_folder}/template.json'
        assert os.path.isfile(new_template_json)
        with open(new_template_json, 'r') as s:
            s_data = s.read()
        assert s_data == template_json_contents

        new_default_name_bus_file = f'{new_template_folder}/Template/Code/Include/' + '${Name}/${Name}Bus.h'
        assert os.path.isfile(new_default_name_bus_file)
        with open(new_default_name_bus_file, 'r') as s:
            s_data = s.read()
        if keep_license_text:
            assert s_data == templated_contents_with_license
        else:
            assert s_data == templated_contents_without_license

        restricted_template_folder = f'{dev_root}/restricted/Salem/Templates'

        new_restricted_template_folder = f'{restricted_template_folder}/Default'
        assert os.path.isdir(new_restricted_template_folder)
        new_restricted_template_json = f'{new_restricted_template_folder}/template.json'
        assert os.path.isfile(new_restricted_template_json)
        with open(new_restricted_template_json, 'r') as s:
            s_data = s.read()
        assert s_data == restricted_template_json_contents

        new_restricted_default_name_bus_file = f'{restricted_template_folder}' \
                                               f'/Default/Template/Code/Include/Platform/Salem/' + '${Name}Bus.h'
        assert os.path.isfile(new_restricted_default_name_bus_file)
        with open(new_restricted_default_name_bus_file, 'r') as s:
            s_data = s.read()
        if keep_license_text:
            assert s_data == templated_contents_with_license
        else:
            assert s_data == templated_contents_without_license


@pytest.mark.parametrize(
    "concrete_contents, templated_contents,"
    " keep_license_text, expect_failure,"
    " template_json_contents, restricted_template_json_contents", [
        pytest.param(TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITH_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                     True, False,
                     TEST_DEFAULTTEMPLATE_JSON_CONTENTS, TEST_DEFAULTTEMPLATE_RESTRICTED_JSON_CONTENTS),
        pytest.param(TEST_CONCRETE_TESTTEMPLATE_CONTENT_WITHOUT_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                     False, False,
                     TEST_DEFAULTTEMPLATE_JSON_CONTENTS, TEST_DEFAULTTEMPLATE_RESTRICTED_JSON_CONTENTS)
    ]
)
def test_create_from_template(tmpdir,
                              concrete_contents, templated_contents,
                              keep_license_text, expect_failure,
                              template_json_contents, restricted_template_json_contents):
    dev_root = str(tmpdir.join('dev').realpath()).replace('\\', '/')
    os.makedirs(dev_root, exist_ok=True)

    template_default_folder = f'{dev_root}/Templates/Default'
    os.makedirs(template_default_folder, exist_ok=True)

    template_json = f'{template_default_folder}/template.json'
    if os.path.isfile(template_json):
        os.unlink(template_json)
    with open(template_json, 'w') as s:
        s.write(template_json_contents)

    default_name_bus_dir = f'{template_default_folder}/Template/Code/Include/' + '${Name}'
    os.makedirs(default_name_bus_dir, exist_ok=True)

    default_name_bus_file = f'{default_name_bus_dir}/' + '${Name}Bus.h'
    if os.path.isfile(default_name_bus_file):
        os.unlink(default_name_bus_file)
    with open(default_name_bus_file, 'w') as s:
        s.write(templated_contents)

    restricted_template_default_folder = f'{dev_root}/restricted/Salem/Templates/Default'
    os.makedirs(restricted_template_default_folder, exist_ok=True)

    restricted_template_json = f'{restricted_template_default_folder}/template.json'
    if os.path.isfile(restricted_template_json):
        os.unlink(restricted_template_json)
    with open(restricted_template_json, 'w') as s:
        s.write(restricted_template_json_contents)

    restricted_default_name_bus_dir = f'{restricted_template_default_folder}/Template/Code/Include/Platform/Salem'
    os.makedirs(restricted_default_name_bus_dir, exist_ok=True)

    restricted_default_name_bus_file = f'{restricted_default_name_bus_dir}/' + '${Name}Bus.h'
    if os.path.isfile(restricted_default_name_bus_file):
        os.unlink(restricted_default_name_bus_file)
    with open(restricted_default_name_bus_file, 'w') as s:
        s.write(templated_contents)

    result = engine_template.create_from_template(dev_root, 'TestTemplate', 'Default',
                                                  keep_license_text=keep_license_text)
    if expect_failure:
        assert result != 0
    else:
        assert result == 0

        test_folder = f'{dev_root}/TestTemplate'
        assert os.path.isdir(test_folder)

        test_bus_file = f'{test_folder}/Code/Include/TestTemplate/TestTemplateBus.h'
        assert os.path.isfile(test_bus_file)
        with open(test_bus_file, 'r') as s:
            s_data = s.read()
        assert s_data == concrete_contents

        restricted_test_bus_folder = f'{dev_root}/restricted/Salem/TestTemplate/Code/Include/Platform/Salem'
        assert os.path.isdir(restricted_test_bus_folder)

        restricted_default_name_bus_file = f'{restricted_test_bus_folder}/TestTemplateBus.h'
        assert os.path.isfile(restricted_default_name_bus_file)
        with open(restricted_default_name_bus_file, 'r') as s:
            s_data = s.read()
        assert s_data == concrete_contents


@pytest.mark.parametrize(
    "concrete_contents, templated_contents,"
    " keep_license_text, expect_failure,"
    " template_json_contents, restricted_template_json_contents", [
        pytest.param(TEST_CONCRETE_TESTPROJECT_TEMPLATE_CONTENT_WITH_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                     True, False,
                     TEST_DEFAULTPROJECT_TEMPLATE_JSON_CONTENTS, TEST_DEFAULTPROJECT_TEMPLATE_RESTRICTED_JSON_CONTENTS),
        pytest.param(TEST_CONCRETE_TESTPROJECT_TEMPLATE_CONTENT_WITHOUT_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                     False, False,
                     TEST_DEFAULTPROJECT_TEMPLATE_JSON_CONTENTS, TEST_DEFAULTPROJECT_TEMPLATE_RESTRICTED_JSON_CONTENTS)
    ]
)
def test_create_project(tmpdir,
                        concrete_contents, templated_contents,
                        keep_license_text, expect_failure,
                        template_json_contents, restricted_template_json_contents):
    dev_root = str(tmpdir.join('dev').realpath()).replace('\\', '/')
    os.makedirs(dev_root, exist_ok=True)

    template_default_folder = f'{dev_root}/Templates/DefaultProject'
    os.makedirs(template_default_folder, exist_ok=True)

    template_json = f'{template_default_folder}/template.json'
    if os.path.isfile(template_json):
        os.unlink(template_json)
    with open(template_json, 'w') as s:
        s.write(template_json_contents)

    default_name_bus_dir = f'{template_default_folder}/Template/Code/Include/' + '${Name}'
    os.makedirs(default_name_bus_dir, exist_ok=True)

    default_name_bus_file = f'{default_name_bus_dir}/' + '${Name}Bus.h'
    if os.path.isfile(default_name_bus_file):
        os.unlink(default_name_bus_file)
    with open(default_name_bus_file, 'w') as s:
        s.write(templated_contents)

    restricted_template_default_folder = f'{dev_root}/restricted/Salem/Templates/DefaultProject'
    os.makedirs(restricted_template_default_folder, exist_ok=True)

    restricted_template_json = f'{restricted_template_default_folder}/template.json'
    if os.path.isfile(restricted_template_json):
        os.unlink(restricted_template_json)
    with open(restricted_template_json, 'w') as s:
        s.write(restricted_template_json_contents)

    restricted_default_name_bus_dir = f'{restricted_template_default_folder}/Template/Code/Include/Platform/Salem'
    os.makedirs(restricted_default_name_bus_dir, exist_ok=True)

    restricted_default_name_bus_file = f'{restricted_default_name_bus_dir}/' + '${Name}Bus.h'
    if os.path.isfile(restricted_default_name_bus_file):
        os.unlink(restricted_default_name_bus_file)
    with open(restricted_default_name_bus_file, 'w') as s:
        s.write(templated_contents)

    result = engine_template.create_project(dev_root, 'TestProject', keep_license_text=keep_license_text)

    if expect_failure:
        assert result != 0
    else:
        assert result == 0

        test_project_folder = f'{dev_root}/TestProject'
        assert os.path.isdir(test_project_folder)

        test_project_bus_file = f'{test_project_folder}/Code/Include/TestProject/TestProjectBus.h'
        assert os.path.isfile(test_project_bus_file)
        with open(test_project_bus_file, 'r') as s:
            s_data = s.read()
        assert s_data == concrete_contents

        restricted_test_project_bus_folder = f'{dev_root}/restricted/Salem/TestProject/Code/Include/Platform/Salem'
        assert os.path.isdir(restricted_test_project_bus_folder)

        restricted_default_name_bus_file = f'{restricted_test_project_bus_folder}/TestProjectBus.h'
        assert os.path.isfile(restricted_default_name_bus_file)
        with open(restricted_default_name_bus_file, 'r') as s:
            s_data = s.read()
        assert s_data == concrete_contents


@pytest.mark.parametrize(
    "concrete_contents, templated_contents,"
    " keep_license_text, expect_failure,"
    " template_json_contents, restricted_template_json_contents", [
        pytest.param(TEST_CONCRETE_TESTGEM_TEMPLATE_CONTENT_WITH_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                     True, False,
                     TEST_DEFAULTGEM_TEMPLATE_JSON_CONTENTS, TEST_DEFAULTGEM_TEMPLATE_RESTRICTED_JSON_CONTENTS),
        pytest.param(TEST_CONCRETE_TESTGEM_TEMPLATE_CONTENT_WITHOUT_LICENSE, TEST_TEMPLATED_CONTENT_WITH_LICENSE,
                     False, False,
                     TEST_DEFAULTGEM_TEMPLATE_JSON_CONTENTS, TEST_DEFAULTGEM_TEMPLATE_RESTRICTED_JSON_CONTENTS)
    ]
)
def test_create_gem(tmpdir,
                    concrete_contents, templated_contents,
                    keep_license_text, expect_failure,
                    template_json_contents, restricted_template_json_contents):
    dev_root = str(tmpdir.join('dev').realpath()).replace('\\', '/')
    os.makedirs(dev_root, exist_ok=True)

    template_default_folder = f'{dev_root}/Templates/DefaultGem'
    os.makedirs(template_default_folder, exist_ok=True)

    template_json = f'{template_default_folder}/template.json'
    if os.path.isfile(template_json):
        os.unlink(template_json)
    with open(template_json, 'w') as s:
        s.write(template_json_contents)

    default_name_bus_dir = f'{template_default_folder}/Template/Code/Include/' + '${Name}'
    os.makedirs(default_name_bus_dir, exist_ok=True)

    default_name_bus_file = f'{default_name_bus_dir}/' + '${Name}Bus.h'
    if os.path.isfile(default_name_bus_file):
        os.unlink(default_name_bus_file)
    with open(default_name_bus_file, 'w') as s:
        s.write(templated_contents)

    restricted_template_default_folder = f'{dev_root}/restricted/Salem/Templates/DefaultGem'
    os.makedirs(restricted_template_default_folder, exist_ok=True)

    restricted_template_json = f'{restricted_template_default_folder}/template.json'
    if os.path.isfile(restricted_template_json):
        os.unlink(restricted_template_json)
    with open(restricted_template_json, 'w') as s:
        s.write(restricted_template_json_contents)

    restricted_default_name_bus_dir = f'{restricted_template_default_folder}/Template/Code/Include/Platform/Salem'
    os.makedirs(restricted_default_name_bus_dir, exist_ok=True)

    restricted_default_name_bus_file = f'{restricted_default_name_bus_dir}/' + '${Name}Bus.h'
    if os.path.isfile(restricted_default_name_bus_file):
        os.unlink(restricted_default_name_bus_file)
    with open(restricted_default_name_bus_file, 'w') as s:
        s.write(templated_contents)

    result = engine_template.create_gem(dev_root, 'TestGem', keep_license_text=keep_license_text)

    if expect_failure:
        assert result != 0
    else:
        assert result == 0

        test_gem_folder = f'{dev_root}/Gems/TestGem'
        assert os.path.isdir(test_gem_folder)

        test_gem_bus_file = f'{test_gem_folder}/Code/Include/TestGem/TestGemBus.h'
        assert os.path.isfile(test_gem_bus_file)
        with open(test_gem_bus_file, 'r') as s:
            s_data = s.read()
        assert s_data == concrete_contents

        restricted_test_gem_bus_folder = f'{dev_root}/restricted/Salem/Gems/TestGem/Code/Include/Platform/Salem'
        assert os.path.isdir(restricted_test_gem_bus_folder)

        restricted_default_name_bus_file = f'{restricted_test_gem_bus_folder}/TestGemBus.h'
        assert os.path.isfile(restricted_default_name_bus_file)
        with open(restricted_default_name_bus_file, 'r') as s:
            s_data = s.read()
        assert s_data == concrete_contents
