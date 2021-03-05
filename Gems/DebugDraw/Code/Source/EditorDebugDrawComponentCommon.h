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

namespace DebugDraw
{
    class EditorDebugDrawComponentSettings
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorDebugDrawComponentSettings, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorDebugDrawComponentSettings, "{39FF3385-9AD8-4C3F-AAFF-3CBE1D76B767}");
        static void Reflect(AZ::ReflectContext* context);

        EditorDebugDrawComponentSettings();

        bool m_visibleInGame;
        bool m_visibleInEditor;
    };
}
