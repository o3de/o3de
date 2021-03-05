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

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace RPI
    {
        //! Settings for custom JSON serializers to get context about loading a file
        class JsonFileLoadContext final
        {
        public:
            AZ_TYPE_INFO(JsonFileLoadContext, "{314942B3-A74A-49D2-822D-CD56F8E3C0F8}");

            void PushFilePath(AZStd::string path);
            AZStd::string_view GetFilePath() const;
            void PopFilePath();

        private:
            // Using vector instead of stack because stack doesn't have a copy constructor
            AZStd::vector<AZStd::string> m_thisFilePath;
        };

    } // namespace RPI
} // namespace AZ
