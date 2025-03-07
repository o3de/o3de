/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/DOM/DomPath.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystemInterface.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/LegacyReflectionBridge.h>

namespace AZ::Reflection
{
    namespace DescriptorAttributes
    {
        const Name Handler = Name::FromStringLiteral("Handler", AZ::Interface<AZ::NameDictionary>::Get());
        const Name Label = Name::FromStringLiteral("Label", AZ::Interface<AZ::NameDictionary>::Get());
        const Name Description = Name::FromStringLiteral("Description", AZ::Interface<AZ::NameDictionary>::Get());
        const Name SerializedPath = Name::FromStringLiteral("SerializedPath", AZ::Interface<AZ::NameDictionary>::Get());
        const Name Container = Name::FromStringLiteral("Container", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ParentContainer = Name::FromStringLiteral("ParentContainer", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ParentContainerInstance = Name::FromStringLiteral("ParentContainerInstance", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ParentContainerCanBeModified = Name::FromStringLiteral("ParentContainerCanBeModified", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ContainerElementOverride = Name::FromStringLiteral("ContainerElementOverride", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ContainerIndex = Name::FromStringLiteral("ContainerIndex", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ParentInstance = Name::FromStringLiteral("ParentInstance", AZ::Interface<AZ::NameDictionary>::Get());
        const Name ParentClassData = Name::FromStringLiteral("ParentClassData", AZ::Interface<AZ::NameDictionary>::Get());
    } // namespace DescriptorAttributes

} // namespace AZ::Reflection
