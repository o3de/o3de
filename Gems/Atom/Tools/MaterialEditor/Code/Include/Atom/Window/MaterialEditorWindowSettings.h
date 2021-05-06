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
    struct MaterialEditorWindowSettings
        : public AZ::UserSettings
    {
        AZ_RTTI(MaterialEditorWindowSettings, "{BB9DEB77-B7BE-4DF5-9FDD-6D9F3136C4EA}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(MaterialEditorWindowSettings, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        bool m_enableGrid = true;
        bool m_enableShadowCatcher = true;
        bool m_enableAlternateSkybox = false;
        float m_fieldOfView = 90.0f;
    };
} // namespace MaterialEditor
