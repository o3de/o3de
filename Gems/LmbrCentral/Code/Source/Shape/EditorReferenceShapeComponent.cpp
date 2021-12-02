/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorReferenceShapeComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include<LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace LmbrCentral
{
    void EditorReferenceShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectSubClass<EditorReferenceShapeComponent, BaseClassType>(context, 1, &EditorWrappedComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType, 1>);
    }
}
