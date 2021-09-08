/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AxisAlignedBoxShape.h"

#include <AzCore/Math/Color.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeDisplay.h>
#include <random>

namespace LmbrCentral
{
    AxisAlignedBoxShape::AxisAlignedBoxShape()
        : BoxShape()
    {
    }

    void AxisAlignedBoxShape::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AxisAlignedBoxShape, BoxShape>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AxisAlignedBoxShape>("Axis Aligned Box Shape", "Axis Aligned Box shape configuration parameters")
                ;
            }
        }
    }

    void AxisAlignedBoxShape::Activate(AZ::EntityId entityId)
    {
        BoxShape::Activate(entityId);
        m_currentTransform.SetRotation(AZ::Quaternion::CreateIdentity());
    }

    void AxisAlignedBoxShape::OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world)
    {
        AZ::Transform worldNoRotation(world.GetTranslation(), AZ::Quaternion::CreateIdentity(), world.GetUniformScale());
        BoxShape::OnTransformChanged(local, worldNoRotation);
    }
} // namespace LmbrCentral
