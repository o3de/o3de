/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

#include <QString>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QTreeWidgetItemIterator::d_ptr': class 'QScopedPointer<QTreeWidgetItemIteratorPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QTreeWidgetItemIterator'
#include <QTreeWidgetItem>
AZ_POP_DISABLE_WARNING
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

class QMimeData;

namespace AZ
{
    class Component;
}

namespace AzToolsFramework
{
    class ComponentTypeMimeData
    {
    public:

        using ClassDataType = const AZ::SerializeContext::ClassData*;
        using ClassDataContainer = AZStd::vector<ClassDataType>;

        static QString GetMimeType();

        static AZStd::unique_ptr<QMimeData> Create(const ClassDataContainer& container);
        static bool Get(const QMimeData* mimeData, ClassDataContainer& container);
    };

    class ComponentMimeData
    {
    public:
        virtual ~ComponentMimeData() = default;

        AZ_RTTI(ComponentMimeData, "{55A643D6-DDE9-4D48-9B6B-B14C46B6C08B}");
        AZ_CLASS_ALLOCATOR(ComponentMimeData, AZ::SystemAllocator);

        using ComponentDataContainer = AZStd::vector<AZ::Component*>;
        static void Reflect(AZ::ReflectContext* context);

        static QString GetMimeType();

        static AZStd::unique_ptr<QMimeData> Create(AZStd::span<AZ::Component* const> components);
        static void GetComponentDataFromMimeData(const QMimeData* mimeData, ComponentDataContainer& componentData);

        static const QMimeData* GetComponentMimeDataFromClipboard();
        static void PutComponentMimeDataOnClipboard(AZStd::unique_ptr<QMimeData> mimeData);

    private:
        ComponentDataContainer m_components;
    };
}
