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

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#endif

namespace MaterialEditor
{
    struct MaterialDocumentSettings
        : public AZ::UserSettings
    {
        AZ_RTTI(MaterialDocumentSettings, "{FA4F4BF3-BF39-4753-AAF7-AF383B868881}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(MaterialDocumentSettings, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        bool m_showReloadDocumentPrompt = true;
        AZStd::string m_defaultMaterialTypeName = "StandardPBR";
    };
} // namespace MaterialEditor
