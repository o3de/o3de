/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DeprecatedNodeLibrary.h"

#include <AzCore/Serialization/EditContext.h>

#include <Libraries/Deprecated/PhysX/WorldNodes.h>
#include <Libraries/Deprecated/Entity/EntityNodes.h>
#include <Libraries/Deprecated/Entity/RotateMethod.h>
#include <Libraries/Deprecated/Math/AABBNodes.h>
#include <Libraries/Deprecated/Math/ColorNodes.h>
#include <Libraries/Deprecated/Math/CRCNodes.h>
#include <Libraries/Deprecated/Math/MathGenerics.h>
#include <Libraries/Deprecated/Math/MathRandom.h>
#include <Libraries/Deprecated/Math/Divide.h>
#include <Libraries/Deprecated/Math/Multiply.h>
#include <Libraries/Deprecated/Math/Matrix3x3Nodes.h>
#include <Libraries/Deprecated/Math/Matrix4x4Nodes.h>
#include <Libraries/Deprecated/Math/OBBNodes.h>
#include <Libraries/Deprecated/Math/PlaneNodes.h>
#include <Libraries/Deprecated/Math/RotationNodes.h>
#include <Libraries/Deprecated/Math/Subtract.h>
#include <Libraries/Deprecated/Math/Sum.h>
#include <Libraries/Deprecated/Math/TransformNodes.h>
#include <Libraries/Deprecated/Math/Vector2Nodes.h>
#include <Libraries/Deprecated/Math/Vector3Nodes.h>
#include <Libraries/Deprecated/Math/Vector4Nodes.h>
#include <Libraries/Deprecated/String/StringGenerics.h>
#include <Libraries/Deprecated/String/StringMethods.h>
#include <Libraries/Libraries.h>

namespace ScriptCanvas
{
    namespace DeprecatedLibrary
    {
        REFLECT_GENERIC_FUNCTION_NODE(Entity, "{C9789111-6A18-4C6E-81A7-87D057D59D2C}");
        REFLECT_GENERIC_FUNCTION_NODE(World, "{55A54AF1-B545-4C12-9F74-01D30789CA1D}");
        REFLECT_GENERIC_FUNCTION_NODE(AABB, "{AB0C2753-680E-47AD-8277-66B3AC01C659}");
        REFLECT_GENERIC_FUNCTION_NODE(Color, "{6CC7B2F9-F551-4CB8-A713-4149959AE337}");
        REFLECT_GENERIC_FUNCTION_NODE(CRC, "{D8B65A5B-38FF-4B41-9FA4-FCA080D75625}");
        REFLECT_GENERIC_FUNCTION_NODE(Math, "{76898795-2B30-4645-B6D4-67568ECC889F}");
        REFLECT_GENERIC_FUNCTION_NODE(Matrix3x3, "{8C5F6959-C2C4-46D9-9FCD-4DC234E7732D}");
        REFLECT_GENERIC_FUNCTION_NODE(Matrix4x4, "{537AB179-E23C-492D-8EF7-53845A2DB163}");
        REFLECT_GENERIC_FUNCTION_NODE(OBB, "{8CBDA3B7-9DCD-4A04-A12C-123C322CA63A}");
        REFLECT_GENERIC_FUNCTION_NODE(Plane, "{F2C799DF-2CC1-4DD8-91DC-18C2D517BAB0}");
        REFLECT_GENERIC_FUNCTION_NODE(Quaternion, "{9BE75E2E-AA07-4767-9BF3-905C289EB38A}");
        REFLECT_GENERIC_FUNCTION_NODE(Random, "{D9DF1385-6C5C-4E41-94CA-8B10E9D8FAAF}");
        REFLECT_GENERIC_FUNCTION_NODE(String, "{5B700838-21A2-4579-9303-F4A4822AFEF4}");
        REFLECT_GENERIC_FUNCTION_NODE(Transform, "{59E0EF87-352C-4CFB-A810-5B8752BDD1EF}");
        REFLECT_GENERIC_FUNCTION_NODE(Vector2, "{FD1BFADF-BA1F-4AFC-819B-ABA1F22AE6E6}");
        REFLECT_GENERIC_FUNCTION_NODE(Vector3, "{53EA7604-E3AE-4E88-BE0E-66CBC488A7DB}");
        REFLECT_GENERIC_FUNCTION_NODE(Vector4, "{66027D7A-FCD1-4592-93E5-EB4B7F4BD671}");
    } // namespace Math

    void DeprecatedNodeLibrary::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            SCRIPT_CANVAS_GENERICS_TO_VM(EntityNodes::Registrar, DeprecatedLibrary::Entity, behaviorContext, EntityNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(ScriptCanvasPhysics::WorldNodes::Registrar, DeprecatedLibrary::World, behaviorContext, ScriptCanvasPhysics::WorldNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(StringNodes::Registrar, DeprecatedLibrary::String, behaviorContext, StringNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(AABBNodes::Registrar, DeprecatedLibrary::AABB, behaviorContext, AABBNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(ColorNodes::Registrar, DeprecatedLibrary::Color, behaviorContext, ColorNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(CRCNodes::Registrar, DeprecatedLibrary::CRC, behaviorContext, CRCNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(MathNodes::Registrar, DeprecatedLibrary::Math, behaviorContext, MathNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(Matrix3x3Nodes::Registrar, DeprecatedLibrary::Matrix3x3, behaviorContext, Matrix3x3Nodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(Matrix4x4Nodes::Registrar, DeprecatedLibrary::Matrix4x4, behaviorContext, Matrix4x4Nodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(OBBNodes::Registrar, DeprecatedLibrary::OBB, behaviorContext, OBBNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(PlaneNodes::Registrar, DeprecatedLibrary::Plane, behaviorContext, PlaneNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(QuaternionNodes::Registrar, DeprecatedLibrary::Quaternion, behaviorContext, QuaternionNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(RandomNodes::Registrar, DeprecatedLibrary::Random, behaviorContext, RandomNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(TransformNodes::Registrar, DeprecatedLibrary::Transform, behaviorContext, TransformNodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(Vector2Nodes::Registrar, DeprecatedLibrary::Vector2, behaviorContext, Vector2Nodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(Vector3Nodes::Registrar, DeprecatedLibrary::Vector3, behaviorContext, Vector3Nodes::k_categoryName);
            SCRIPT_CANVAS_GENERICS_TO_VM(Vector4Nodes::Registrar, DeprecatedLibrary::Vector4, behaviorContext, Vector4Nodes::k_categoryName);
        }

        Entity::RotateMethod::Reflect(reflection);
        StringMethods::Reflect(reflection);
    }

    AZStd::vector<AZ::ComponentDescriptor*> DeprecatedNodeLibrary::GetComponentDescriptors()
    {
        AZStd::vector<AZ::ComponentDescriptor*> descriptors = {
            ScriptCanvas::Nodes::Math::Divide::CreateDescriptor(),
            ScriptCanvas::Nodes::Math::Multiply::CreateDescriptor(),
            ScriptCanvas::Nodes::Math::Subtract::CreateDescriptor(),
            ScriptCanvas::Nodes::Math::Sum::CreateDescriptor(),
        };

        EntityNodes::Registrar::AddDescriptors(descriptors);
        ScriptCanvasPhysics::WorldNodes::Registrar::AddDescriptors(descriptors);
        StringNodes::Registrar::AddDescriptors(descriptors);
        AABBNodes::Registrar::AddDescriptors(descriptors);
        ColorNodes::Registrar::AddDescriptors(descriptors);
        CRCNodes::Registrar::AddDescriptors(descriptors);
        MathNodes::Registrar::AddDescriptors(descriptors);
        Matrix3x3Nodes::Registrar::AddDescriptors(descriptors);
        Matrix4x4Nodes::Registrar::AddDescriptors(descriptors);
        OBBNodes::Registrar::AddDescriptors(descriptors);
        PlaneNodes::Registrar::AddDescriptors(descriptors);
        QuaternionNodes::Registrar::AddDescriptors(descriptors);
        RandomNodes::Registrar::AddDescriptors(descriptors);
        TransformNodes::Registrar::AddDescriptors(descriptors);
        Vector2Nodes::Registrar::AddDescriptors(descriptors);
        Vector3Nodes::Registrar::AddDescriptors(descriptors);
        Vector4Nodes::Registrar::AddDescriptors(descriptors);

        return descriptors;
    }
} // namespace ScriptCanvas
