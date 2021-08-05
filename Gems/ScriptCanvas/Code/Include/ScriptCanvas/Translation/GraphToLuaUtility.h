/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
