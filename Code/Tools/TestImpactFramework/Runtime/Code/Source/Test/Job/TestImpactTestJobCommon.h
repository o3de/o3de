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

#include <TestImpactFramework/TestImpactException.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    template<typename ExceptionType>
    AZStd::string ReadFileContents(const AZ::IO::Path& file)
    {
        static_assert(AZStd::is_base_of<Exception, ExceptionType>::value, "Exception must be a TestImpact exception or derived type");

        const auto fileSize = AZ::IO::SystemFile::Length(file.c_str());
        AZ_TestImpact_Eval(fileSize > 0, ExceptionType, AZStd::string::format("File %s does not exist", file.c_str()));

        AZStd::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = '\0';
        AZ_TestImpact_Eval(AZ::IO::SystemFile::Read(file.c_str(), buffer.data()), ExceptionType, "Could not read file contents");

        return AZStd::string(buffer.begin(), buffer.end());
    }
} // namespace TestImpact
