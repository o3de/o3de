/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/PropertyTypes.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Physics
{
    void ShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShapeConfiguration>()
                ->Version(1)
                ->Field("Scale", &ShapeConfiguration::m_scale)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            #define REFLECT_SHAPETYPE_ENUM_VALUE(EnumValue)                                                                                            \
                behaviorContext->EnumProperty<(int)Physics::ShapeType::EnumValue>("ShapeType_"#EnumValue)                                                      \
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)                                         \
                    ->Attribute(AZ::Script::Attributes::Module, "physics");

            // Note: Here we only expose the types that are available to the user in the editor
            REFLECT_SHAPETYPE_ENUM_VALUE(Box);
            REFLECT_SHAPETYPE_ENUM_VALUE(Sphere);
            REFLECT_SHAPETYPE_ENUM_VALUE(Cylinder);
            REFLECT_SHAPETYPE_ENUM_VALUE(PhysicsAsset);

            #undef REFLECT_SHAPETYPE_ENUM_VALUE
        }
    }

    void SphereShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->RegisterGenericType<AZStd::shared_ptr<SphereShapeConfiguration>>();

            serializeContext->Class<SphereShapeConfiguration, ShapeConfiguration>()
                ->Version(1)
                ->Field("Radius", &SphereShapeConfiguration::m_radius)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SphereShapeConfiguration>("SphereShapeConfiguration", "Configuration for sphere collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SphereShapeConfiguration::m_radius, "Radius", "The radius of the sphere collider")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ;
            }
        }
    }

    SphereShapeConfiguration::SphereShapeConfiguration(float radius)
        : m_radius(radius)
    {
    }

    void BoxShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->RegisterGenericType<AZStd::shared_ptr<BoxShapeConfiguration>>();

            serializeContext->Class<BoxShapeConfiguration, ShapeConfiguration>()
                ->Version(1)
                ->Field("Configuration", &BoxShapeConfiguration::m_dimensions)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<BoxShapeConfiguration>("BoxShapeConfiguration", "Configuration for box collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &BoxShapeConfiguration::m_dimensions, "Dimensions", "Lengths of the box sides")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ;
            }
        }
    }

    BoxShapeConfiguration::BoxShapeConfiguration(const AZ::Vector3& boxDimensions)
        : m_dimensions(boxDimensions)
    {
    }

    void CapsuleShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->RegisterGenericType<AZStd::shared_ptr<CapsuleShapeConfiguration>>();

            serializeContext->Class<CapsuleShapeConfiguration, ShapeConfiguration>()
                ->Version(1)
                ->Field("Height", &CapsuleShapeConfiguration::m_height)
                ->Field("Radius", &CapsuleShapeConfiguration::m_radius)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CapsuleShapeConfiguration>("CapsuleShapeConfiguration", "Configuration for capsule collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShapeConfiguration::m_height, "Height", "The height of the capsule, including caps at each end")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CapsuleShapeConfiguration::OnHeightChanged)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShapeConfiguration::m_radius, "Radius", "The radius of the capsule")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &CapsuleShapeConfiguration::OnRadiusChanged)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                    ;
            }
        }
    }

    CapsuleShapeConfiguration::CapsuleShapeConfiguration(float height, float radius)
        : m_height(height)
        , m_radius(radius)
    {
    }

    void CapsuleShapeConfiguration::OnHeightChanged()
    {
        // check that the height is greater than twice the radius
        m_height = AZ::GetMax(m_height, 2 * m_radius + AZ::Constants::FloatEpsilon);
    }

    void CapsuleShapeConfiguration::OnRadiusChanged()
    {
        // check that the radius is less than half the height
        m_radius = AZ::GetMin(m_radius, (0.5f - AZ::Constants::FloatEpsilon) * m_height);
    }

    void PhysicsAssetShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->RegisterGenericType<AZStd::shared_ptr<PhysicsAssetShapeConfiguration>>();

            serializeContext->Class<PhysicsAssetShapeConfiguration, ShapeConfiguration>()
                ->Version(3)
                ->Field("PhysicsAsset", &PhysicsAssetShapeConfiguration::m_asset)
                ->Field("AssetScale", &PhysicsAssetShapeConfiguration::m_assetScale)
                ->Field("UseMaterialsFromAsset", &PhysicsAssetShapeConfiguration::m_useMaterialsFromAsset)
                ->Field("SubdivisionLevel", &PhysicsAssetShapeConfiguration::m_subdivisionLevel)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PhysicsAssetShapeConfiguration>("PhysicsAssetShapeConfiguration", "Configuration for asset shape collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysicsAssetShapeConfiguration::m_assetScale, "Asset Scale", "The scale of the asset shape")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PhysicsAssetShapeConfiguration::m_useMaterialsFromAsset, "Physics materials from asset", "Auto-set physics materials using asset's physics material names")
                    ;
            }
        }
    }

    Physics::ShapeType PhysicsAssetShapeConfiguration::GetShapeType() const
    {
        return ShapeType::PhysicsAsset;
    }

    void NativeShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->RegisterGenericType<AZStd::shared_ptr<NativeShapeConfiguration>>();

            serializeContext->Class<NativeShapeConfiguration, ShapeConfiguration>()
                ->Version(1)
                ->Field("Scale", &NativeShapeConfiguration::m_nativeShapeScale)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NativeShapeConfiguration>("NativeShapeConfiguration", "Configuration for native shape collider")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &NativeShapeConfiguration::m_nativeShapeScale, "Scale", "The scale of the native shape")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                    ;
            }
        }
    }

    void CookedMeshShapeConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext
                ->RegisterGenericType<AZStd::shared_ptr<CookedMeshShapeConfiguration>>();

            serializeContext->Class<CookedMeshShapeConfiguration, ShapeConfiguration>()
                ->Version(1)
                ->Field("CookedData", &CookedMeshShapeConfiguration::m_cookedData)
                ->Field("Type", &CookedMeshShapeConfiguration::m_type)
                ;
        }
    }

    CookedMeshShapeConfiguration::~CookedMeshShapeConfiguration()
    {
        ReleaseCachedNativeMesh();
    }

    CookedMeshShapeConfiguration::CookedMeshShapeConfiguration(const CookedMeshShapeConfiguration& other)
        : ShapeConfiguration(other)
        , m_cookedData(other.m_cookedData)
        , m_type(other.m_type)
        , m_cachedNativeMesh(nullptr)
    {
    }

    CookedMeshShapeConfiguration& CookedMeshShapeConfiguration::operator=(const CookedMeshShapeConfiguration& other)
    {
        ShapeConfiguration::operator=(other);

        m_cookedData = other.m_cookedData;
        m_type = other.m_type;

        // Prevent raw pointer from being copied
        m_cachedNativeMesh = nullptr;

        return *this;
    }

    ShapeType CookedMeshShapeConfiguration::GetShapeType() const
    {
        return ShapeType::CookedMesh;
    }

    void CookedMeshShapeConfiguration::SetCookedMeshData(const AZ::u8* cookedData, 
        size_t cookedDataSize, MeshType type)
    {
        // If the new cooked data is being set, make sure we clear the cached mesh
        ReleaseCachedNativeMesh();
        
        m_cookedData.clear();
        m_cookedData.insert(m_cookedData.end(), cookedData, cookedData + cookedDataSize);
        
        m_type = type;
    }

    const AZStd::vector<AZ::u8>& CookedMeshShapeConfiguration::GetCookedMeshData() const
    {
        return m_cookedData;
    }

    CookedMeshShapeConfiguration::MeshType CookedMeshShapeConfiguration::GetMeshType() const
    {
        return m_type;
    }

    void* CookedMeshShapeConfiguration::GetCachedNativeMesh() const
    {
        return m_cachedNativeMesh;
    }

    void CookedMeshShapeConfiguration::SetCachedNativeMesh(void* cachedNativeMesh) const
    {
        m_cachedNativeMesh = cachedNativeMesh;
    }

    void CookedMeshShapeConfiguration::ReleaseCachedNativeMesh()
    {
        if (m_cachedNativeMesh)
        {
            Physics::SystemRequestBus::Broadcast(
                &Physics::SystemRequests::ReleaseNativeMeshObject, m_cachedNativeMesh);

            m_cachedNativeMesh = nullptr;
        }
    }
}
