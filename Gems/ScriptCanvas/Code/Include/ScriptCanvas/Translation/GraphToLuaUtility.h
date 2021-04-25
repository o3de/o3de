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

#include <AzCore/Outcome/Outcome.h>

#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

#include "TranslationUtilities.h"
#include "GraphToX.h"

namespace ScriptCanvas
{
    class Datum;

    namespace Data
    {
        class Type;
    }

    namespace Translation
    {
        class GraphToLua;

        void CheckConversionStringPost(Writer& writer, Grammar::VariableConstPtr source, const Grammar::ConversionByIndex& conversions, size_t index);

        void CheckConversionStringPre(Writer& writer, Grammar::VariableConstPtr source, const Grammar::ConversionByIndex& conversions, size_t index);

        bool IsReferenceInLuaAndValueInScriptCanvas(const Data::Type& type);

        AZStd::string ToValueString(const Datum& datum, const Configuration& config);
    } 

}
