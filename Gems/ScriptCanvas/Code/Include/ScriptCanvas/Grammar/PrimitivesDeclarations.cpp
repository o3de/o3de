/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        AZ_CVAR(bool, g_printAbstractCodeModel, true, {}, AZ::ConsoleFunctorFlags::Null, "Print out the Abstract Code Model at the end of parsing for debug purposes.");
        AZ_CVAR(bool, g_printAbstractCodeModelAtPrefabTime, false, {}, AZ::ConsoleFunctorFlags::Null, "Print out the Abstract Code Model at the end of parsing (at prefab time) for debug purposes.");
        AZ_CVAR(bool, g_saveRawTranslationOuputToFile, true, {}, AZ::ConsoleFunctorFlags::Null, "Save out the raw result of translation for debug purposes.");
        AZ_CVAR(bool, g_saveRawTranslationOuputToFileAtPrefabTime, false, {}, AZ::ConsoleFunctorFlags::Null, "Save out the raw result of translation (at prefab time) for debug purposes.");

        SettingsCache::SettingsCache()
        {
            m_disableParseOnGraphValidation = g_disableParseOnGraphValidation;
            m_printAbstractCodeModel = g_printAbstractCodeModel;
            m_printAbstractCodeModelAtPrefabTime = g_printAbstractCodeModelAtPrefabTime;
            m_saveRawTranslationOuputToFile = g_saveRawTranslationOuputToFile;
            m_saveRawTranslationOuputToFileAtPrefabTime = g_saveRawTranslationOuputToFileAtPrefabTime;
        }

        SettingsCache::~SettingsCache()
        {
            g_disableParseOnGraphValidation = m_disableParseOnGraphValidation;
            g_printAbstractCodeModel = m_printAbstractCodeModel;
            g_printAbstractCodeModelAtPrefabTime = m_printAbstractCodeModelAtPrefabTime;
            g_saveRawTranslationOuputToFile = m_saveRawTranslationOuputToFile;
            g_saveRawTranslationOuputToFileAtPrefabTime = m_saveRawTranslationOuputToFileAtPrefabTime;
        }
    }
}
