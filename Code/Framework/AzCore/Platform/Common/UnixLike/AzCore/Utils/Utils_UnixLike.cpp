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

#include <AzCore/Utils/Utils.h>

#include <stdlib.h>

namespace AZ
{
    namespace Utils
    {
        void RequestAbnormalTermination()
        {
            abort();
        }

        void NativeErrorMessageBox(const char*, const char*) {}

        AZStd::optional<AZStd::fixed_string<MaxPathLength>> ConvertToAbsolutePath(AZStd::string_view path)
        {
            AZStd::fixed_string<MaxPathLength> absolutePath;
            AZStd::fixed_string<MaxPathLength> srcPath{ path };

            if (char* result = realpath(srcPath.c_str(), absolutePath.data()); result)
            {
                // Fix the size value of the fixed string by calculating the c-string length using char traits
                absolutePath.resize_no_construct(AZStd::char_traits<char>::length(absolutePath.data()));
                return absolutePath;
            }

            return AZStd::nullopt;
        }
    } // namespace Utils
} // namespace AZ
