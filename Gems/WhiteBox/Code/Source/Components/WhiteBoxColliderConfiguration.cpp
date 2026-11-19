/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxColliderConfiguration.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(WhiteBoxColliderConfiguration, AZ::SystemAllocator)

    AZ::Crc32 WhiteBoxColliderConfiguration::SimplifiedVisibility() const
    {
        return m_shape == WhiteBoxColliderShape::SimplifiedMesh ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    AZ::Crc32 WhiteBoxColliderConfiguration::PartsVisibility() const
    {
        return m_shape == WhiteBoxColliderShape::ConvexParts ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void WhiteBoxColliderConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxColliderConfiguration>()
                ->Version(4)
                ->Field("BodyType", &WhiteBoxColliderConfiguration::m_bodyType)
                ->Field("Shape", &WhiteBoxColliderConfiguration::m_shape)
                ->Field("MaxHullsPerShell", &WhiteBoxColliderConfiguration::m_maxHullsPerShell)
                ->Field("DecompositionResolution", &WhiteBoxColliderConfiguration::m_decompositionResolution)
                ->Field("MaxVerticesPerHull", &WhiteBoxColliderConfiguration::m_maxVerticesPerHull)
                ->Field("MeshResolution", &WhiteBoxColliderConfiguration::m_meshResolution);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext
                    ->Class<WhiteBoxColliderConfiguration>(
                        QT_TRANSLATE_NOOP("WhiteBox", "White Box Collider Configuration"),
                        QT_TRANSLATE_NOOP("WhiteBox", "White Box collider configuration properties"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &WhiteBoxColliderConfiguration::m_bodyType,
                        QT_TRANSLATE_NOOP("WhiteBox", "Body Type"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Set if the White Box Collider will be treated as static or kinematic at runtime."))
                    ->EnumAttribute(WhiteBoxBodyType::Static, QT_TRANSLATE_NOOP("WhiteBox", "Static"))
                    ->EnumAttribute(WhiteBoxBodyType::Kinematic, QT_TRANSLATE_NOOP("WhiteBox", "Kinematic"))
                    ->DataElement(
                        AZ::Edit::UIHandlers::ComboBox, &WhiteBoxColliderConfiguration::m_shape,
                        QT_TRANSLATE_NOOP("WhiteBox", "Collision Shape"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Triangle Mesh follows every face exactly. Triangle Mesh (Simplified) keeps only the share of triangles "
                        "set by Resolution. Convex Hull wraps the whole mesh in one convex shape. "
                        "Convex Parts gives each separate shell (layer or island) its own hull and splits concave ones, so "
                        "doorways and U shapes stay open. PhysX triggers must be convex, so Trigger with Triangle Mesh cooks a hull."))
                    ->EnumAttribute(WhiteBoxColliderShape::TriangleMesh, QT_TRANSLATE_NOOP("WhiteBox", "Triangle Mesh"))
                    ->EnumAttribute(WhiteBoxColliderShape::SimplifiedMesh,
                    QT_TRANSLATE_NOOP("WhiteBox", "Triangle Mesh (Simplified)"))
                    ->EnumAttribute(WhiteBoxColliderShape::ConvexHull,
                    QT_TRANSLATE_NOOP("WhiteBox", "Convex Hull (Single)"))
                    ->EnumAttribute(WhiteBoxColliderShape::ConvexParts, QT_TRANSLATE_NOOP("WhiteBox", "Convex Parts"))
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Slider, &WhiteBoxColliderConfiguration::m_meshResolution,
                        QT_TRANSLATE_NOOP("WhiteBox", "Resolution"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Percent of the mesh's triangles the collider keeps. Flat areas and gentle curves go first; faces are "
                        "never folded over, so edges stay where they are."))
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 100)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " %")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &WhiteBoxColliderConfiguration::SimplifiedVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::SpinBox, &WhiteBoxColliderConfiguration::m_maxHullsPerShell,
                        QT_TRANSLATE_NOOP("WhiteBox", "Max Hulls Per Shell"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Most hulls a concave shell may split into. Closed shells are cut cleanly along their own faces; "
                        "one that is open or would need more hulls than this falls back to V-HACD's approximation."))
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, 256)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &WhiteBoxColliderConfiguration::PartsVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::SpinBox, &WhiteBoxColliderConfiguration::m_decompositionResolution,
                        QT_TRANSLATE_NOOP("WhiteBox", "Resolution"),
                        QT_TRANSLATE_NOOP("WhiteBox", "V-HACD fallback only: voxels it samples each concave shell with. Higher follows small detail more closely but cooks slower."))
                    ->Attribute(AZ::Edit::Attributes::Min, 10000)
                    ->Attribute(AZ::Edit::Attributes::Max, 10000000)
                    ->Attribute(AZ::Edit::Attributes::Step, 10000)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &WhiteBoxColliderConfiguration::PartsVisibility)
                    ->DataElement(
                        AZ::Edit::UIHandlers::SpinBox, &WhiteBoxColliderConfiguration::m_maxVerticesPerHull,
                        QT_TRANSLATE_NOOP("WhiteBox", "Max Vertices Per Hull"),
                        QT_TRANSLATE_NOOP("WhiteBox", "V-HACD fallback only: vertex cap for each generated hull (PhysX allows at most 255)."))
                    ->Attribute(AZ::Edit::Attributes::Min, 8)
                    ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &WhiteBoxColliderConfiguration::PartsVisibility);
            }
        }
    }
} // namespace WhiteBox
