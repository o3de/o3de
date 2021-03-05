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

#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>

#include <GraphCanvas/Styling/Style.h>

namespace GraphCanvas
{
    class StyleManager;

    namespace Styling
    {
        class Parser
        {
        public:
            static void Parse(StyleManager& styleManager, const AZStd::string& json);
            static void Parse(StyleManager& styleManager, const rapidjson::Document& json);

        private:

            static void ParseStyle(StyleManager& styleManager, const rapidjson::Value& value);

            static SelectorVector ParseSelectors(const rapidjson::Value& value);
        };

    } // namespace Styling
} // namespace GraphCanvas
