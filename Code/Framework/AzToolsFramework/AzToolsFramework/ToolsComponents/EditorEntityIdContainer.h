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
#ifndef EDITOR_ENTITY_ID_CONTAINER_H
#define EDITOR_ENTITY_ID_CONTAINER_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>

class QString;

namespace AZ
{
    struct ClassDataReflection;
}

namespace AzToolsFramework
{
    class EditorEntityIdContainer
    {
    public:
        virtual ~EditorEntityIdContainer() { }

        AZ_RTTI(EditorEntityIdContainer, "{22F4C72A-8ADD-49B3-884C-30C7F254AAC6}");
        AZ_CLASS_ALLOCATOR(EditorEntityIdContainer, AZ::SystemAllocator, 0);

        static const QString& GetMimeType();

        AZStd::vector< AZ::EntityId > m_entityIds;

        // utility functions to serialize/deserialize.
        bool ToBuffer(AZStd::vector<char>& buffer);
        bool FromBuffer(const AZStd::vector<char>& buffer);
        bool FromBuffer(const char* data, AZStd::size_t size);

        static void Reflect(AZ::ReflectContext* context);
    };
}

#endif // EDITOR_ENTITY_ID_LIST_CONTAINER_H
