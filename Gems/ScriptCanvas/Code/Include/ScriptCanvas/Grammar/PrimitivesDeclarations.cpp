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

#include "PrimitivesDeclarations.h"

namespace ScriptCanvas
{
    namespace Grammar
    {
        AZ_CVAR(bool, g_disableParseOnGraphValidation, false, {}, AZ::ConsoleFunctorFlags::Null, "In case parsing the graph is interfering with opening a graph, disable parsing on validation");
        AZ_CVAR(bool, g_printAbstractCodeModel, false, {}, AZ::ConsoleFunctorFlags::Null, "Print out the Abstract Code Model at the end of parsing for debug purposes.");
        AZ_CVAR(bool, g_saveRawTranslationOuputToFile, false, {}, AZ::ConsoleFunctorFlags::Null, "Save out the raw result of translation for debug purposes.");
    }
}
