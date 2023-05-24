/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(EditorEntityIdContainer, AZ::SystemAllocator);

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
