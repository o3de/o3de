/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
