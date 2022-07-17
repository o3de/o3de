/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponentModeBus.h"
#include "WhiteBoxToolApiReflection.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>
#include <WhiteBox/WhiteBoxComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    // placeholder type for white box free functions
    struct WhiteBoxUtil
    {
    };

    WhiteBoxMesh* WhiteBoxMeshFromHandle(const WhiteBoxMeshHandle& whiteBoxMeshHandle)
    {
        return reinterpret_cast<WhiteBox::WhiteBoxMesh*>(whiteBoxMeshHandle.m_whiteBoxMeshAddress);
    }

    template<typename Tag>
    void GenericHandleReflect(AZ::BehaviorContext* behaviorContext, const char* name)
    {
        behaviorContext->Class<typename WhiteBox::GenericHandle<Tag>>(name)
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
            ->Attribute(AZ::Script::Attributes::Module, "whitebox.api")
            ->template Constructor<int>()
            ->Method("IsValid", &GenericHandle<Tag>::IsValid)
            ->Method("Index", &GenericHandle<Tag>::Index);
    }

    void Reflect(AZ::ReflectContext* context)
    {
        // SerializeContext registration is needed to convert python lists into AZStd::vector
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Api::FaceHandle>();
            serializeContext->RegisterGenericType<Api::FaceHandles>();

            serializeContext->Class<Api::FaceVertHandles>();
            serializeContext->RegisterGenericType<Api::FaceVertHandlesList>();

            serializeContext->Class<Api::VertexHandle>();
            serializeContext->RegisterGenericType<Api::VertexHandles>();

            serializeContext->RegisterGenericType<AZStd::array<Api::VertexHandle, 3>>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            GenericHandleReflect<Api::VertexHandleTag>(behaviorContext, "VertexHandle");
            GenericHandleReflect<Api::FaceHandleTag>(behaviorContext, "FaceHandle");
            GenericHandleReflect<Api::EdgeHandleTag>(behaviorContext, "EdgeHandle");
            GenericHandleReflect<Api::HalfedgeHandleTag>(behaviorContext, "HalfedgeHandle");

            behaviorContext->Class<Api::FaceVertHandles>("FaceVertHandles")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.api")
                ->Property("VertexHandles", BehaviorValueProperty(&Api::FaceVertHandles::m_vertexHandles));

            behaviorContext->Class<Api::EdgeTypes>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.api")
                ->Property("Mesh", BehaviorValueProperty(&Api::EdgeTypes::m_mesh))
                ->Property("User", BehaviorValueProperty(&Api::EdgeTypes::m_user));

            behaviorContext->Class<Api::PolygonHandle>("PolygonHandle")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.api")
                ->Property("FaceHandles", BehaviorValueProperty(&Api::PolygonHandle::m_faceHandles));

            behaviorContext->Class<WhiteBoxMeshHandle>("WhiteBoxMeshHandle")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.api")
                ->Method("IsValid", [](WhiteBoxMeshHandle* whiteBoxMeshHandle) {
                    return (WhiteBoxMeshFromHandle(*whiteBoxMeshHandle) != nullptr);
                })
                ->Method(
                    "InitializeAsUnitCube",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if (whiteboxMesh)
                        {
                            return Api::InitializeAsUnitCube(*whiteboxMesh);
                        }
                        return Api::PolygonHandles {};
                    })
                ->Method(
                    "MeshFaceCount",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if (whiteboxMesh)
                        {
                            return Api::MeshFaceCount(*whiteboxMesh);
                        }
                        return std::numeric_limits<size_t>::min();
                    })
                ->Method(
                    "MeshVertexCount",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle)
                    {
                        return Api::MeshVertexCount(*WhiteBoxMeshFromHandle(*whiteBoxMeshHandle));
                    })
                ->Method(
                    "FacePolygonHandle",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, const Api::FaceHandle faceHandle)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if (whiteboxMesh)
                        {
                            return Api::FacePolygonHandle(*whiteboxMesh, faceHandle);
                        }
                        return Api::PolygonHandle {};
                    })
                ->Method(
                    "FaceVertexHandles",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, const Api::FaceHandle faceHandle)
                    {
                        return Api::FaceVertexHandles(*WhiteBoxMeshFromHandle(*whiteBoxMeshHandle), faceHandle);
                    })
                ->Method(
                    "AddVertex",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, const AZ::Vector3& vertex)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if (whiteboxMesh)
                        {
                            return Api::AddVertex(*whiteboxMesh, vertex);
                        }
                        return Api::VertexHandle{};
                    })
                ->Method(
                    "VertexPosition",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, Api::VertexHandle vertexHandle)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if (whiteboxMesh) 
                        {
                            return Api::VertexPosition(*whiteboxMesh, vertexHandle);
                        }
                        return AZ::Vector3::CreateZero();
                    })
                ->Method(
                    "VertexPositions",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, const Api::VertexHandles& vertexHandles)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if(whiteboxMesh) 
                        {
                            return Api::VertexPositions(*whiteboxMesh, vertexHandles);
                        }
                        return AZStd::vector<AZ::Vector3>{};
                    })
                ->Method(
                    "TranslatePolygonAppend",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, const Api::PolygonHandle& polygonHandle, const float distance)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if(whiteboxMesh) 
                        {
                            return Api::TranslatePolygonAppend(*whiteboxMesh, polygonHandle, distance);
                        }
                        return Api::PolygonHandle {};
                    })
                ->Method(
                    "TranslatePolygon",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, const Api::PolygonHandle& polygonHandle, const float distance)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if(whiteboxMesh) 
                        {
                            Api::TranslatePolygon(*whiteboxMesh, polygonHandle, distance);
                        }
                    })
                ->Method(
                    "CalculateNormals",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if(whiteboxMesh) 
                        {
                            Api::CalculateNormals(*whiteboxMesh);
                        }
                    })
                ->Method(
                    "CalculatePlanarUVs",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if(whiteboxMesh) 
                        {
                            Api::CalculatePlanarUVs(*whiteboxMesh);
                        }
                    })
                ->Method(
                    "HideEdge",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, Api::EdgeHandle edgeHandle)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if(whiteboxMesh) 
                        {
                            return Api::HideEdge(*whiteboxMesh, edgeHandle);
                        }
                        return Api::PolygonHandle{};
                    })
                ->Method(
                    "FlipEdge",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, Api::EdgeHandle edgeHandle)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if(whiteboxMesh) 
                        {
                            return Api::FlipEdge(*whiteboxMesh, edgeHandle);
                        }
                        return false;
                    })
                ->Method(
                    "AddPolygon",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, const Api::FaceVertHandlesList& faceVertHandles)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if(whiteboxMesh) 
                        {
                            return Api::AddPolygon(*whiteboxMesh, faceVertHandles);
                        }
                        return Api::PolygonHandle {};
                    })
                ->Method(
                    "AddTriPolygon",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle, Api::VertexHandle v0, Api::VertexHandle v1, Api::VertexHandle v2)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if(whiteboxMesh) 
                        {
                            return Api::AddTriPolygon(*whiteboxMesh, v0, v1, v2);
                        }
                        return Api::PolygonHandle {};
                    })
                ->Method(
                    "AddQuadPolygon",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle,
                       Api::VertexHandle v0,
                       Api::VertexHandle v1,
                       Api::VertexHandle v2,
                       Api::VertexHandle v3)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.");
                        if(whiteboxMesh) 
                        {
                            return Api::AddQuadPolygon(*whiteboxMesh, v0, v1, v2, v3);
                        }
                        return Api::PolygonHandle {};
                    })
                ->Method(
                    "Clear",
                    [](WhiteBoxMeshHandle* whiteBoxMeshHandle)
                    {
                        WhiteBoxMesh* whiteboxMesh = WhiteBoxMeshFromHandle(*whiteBoxMeshHandle);
                        AZ_Assert(whiteboxMesh, "WhiteBoxMesh is not found.")
                        if(whiteboxMesh) 
                        {
                            Api::Clear(*whiteboxMesh);
                        }
                    });

            behaviorContext->EnumProperty<(int)DefaultShapeType::Cube>("CUBE")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.enum");

            behaviorContext->EnumProperty<(int)DefaultShapeType::Tetrahedron>("TETRAHEDRON")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.enum");

            behaviorContext->EnumProperty<(int)DefaultShapeType::Icosahedron>("ICOSAHEDRON")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.enum");

            behaviorContext->EnumProperty<(int)DefaultShapeType::Cylinder>("CYLINDER")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.enum");

            behaviorContext->EnumProperty<(int)DefaultShapeType::Sphere>("SPHERE")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.enum");

            behaviorContext->EBus<EditorWhiteBoxComponentModeRequestBus>("EditorWhiteBoxComponentModeRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.request.bus")
                ->Event(
                    "MarkWhiteBoxIntersectionDataDirty",
                    &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);

            behaviorContext->EBus<EditorWhiteBoxComponentRequestBus>("EditorWhiteBoxComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.request.bus")
                ->Event("GetWhiteBoxMeshHandle", &EditorWhiteBoxComponentRequestBus::Events::GetWhiteBoxMeshHandle)
                ->Event("SerializeWhiteBox", &EditorWhiteBoxComponentRequestBus::Events::SerializeWhiteBox)
                ->Event("SetDefaultShape", &EditorWhiteBoxComponentRequestBus::Events::SetDefaultShape);

            behaviorContext->EBus<EditorWhiteBoxComponentNotificationBus>("EditorWhiteBoxComponentNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.notification.bus")
                ->Event(
                    "OnWhiteBoxMeshModified", &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);

            behaviorContext->EBus<WhiteBoxComponentRequestBus>("WhiteBoxComponentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.request.bus")
                ->Event("WhiteBoxIsVisible", &WhiteBoxComponentRequestBus::Events::WhiteBoxIsVisible);

            behaviorContext->Class<WhiteBoxUtil>("util")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "whitebox.api")
                ->Method(
                    "MakeFaceVertHandles",
                    [](Api::VertexHandle v0, Api::VertexHandle v1, Api::VertexHandle v2)
                    {
                        return Api::FaceVertHandles{v0, v1, v2};
                    })
                ->Method(
                    "MakeEntityComponentIdPair",
                    [](AZ::u64 entityId, AZ::u64 componentId)
                    {
                        return AZ::EntityComponentIdPair(AZ::EntityId(entityId), componentId);
                    });
        }
    }
} // namespace WhiteBox

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(WhiteBox::WhiteBoxMeshHandle, "{95A2A7F0-C758-494E-BE1E-F673D13E812D}")

    AZ_TYPE_INFO_SPECIALIZE(
        WhiteBox::GenericHandle<WhiteBox::Api::VertexHandleTag>, "{708A5B3C-E377-40CE-9572-BEB64C849D40}")
    AZ_TYPE_INFO_SPECIALIZE(
        WhiteBox::GenericHandle<WhiteBox::Api::FaceHandleTag>, "{950009BC-8991-4749-9D5C-08C62AF34E7B}")
    AZ_TYPE_INFO_SPECIALIZE(
        WhiteBox::GenericHandle<WhiteBox::Api::EdgeHandleTag>, "{2169BFE7-8676-4572-B57E-494577059FB5}")
    AZ_TYPE_INFO_SPECIALIZE(
        WhiteBox::GenericHandle<WhiteBox::Api::HalfedgeHandleTag>, "{50AD7640-2A57-4311-B3DE-7BB08B1B70E5}")

    AZ_TYPE_INFO_SPECIALIZE(WhiteBox::Api::PolygonHandle, "{CE09B0D7-3076-4EAC-ADA7-7418A31EE9AE}")
    AZ_TYPE_INFO_SPECIALIZE(WhiteBox::Api::EdgeTypes, "{15581F2B-E80B-4264-AE26-659B5D214552}")
    AZ_TYPE_INFO_SPECIALIZE(WhiteBox::Api::FaceVertHandles, "{F6B9150B-CC89-48A2-AB89-D18740CC6FA2}")

    AZ_TYPE_INFO_SPECIALIZE(WhiteBox::WhiteBoxUtil, "{8D46CA40-1B9A-440E-A9A3-0CDD5773BB4A}")
} // namespace AZ
