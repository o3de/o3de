/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Math.h"

#include <ScriptCanvas/Internal/Nodes/ExpressionNodeBase.h>

#pragma warning (disable:4503) // decorated name length exceeded, name was truncated

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            REFLECT_MATH_LIBRARY_TYPE(AABB, "{AB0C2753-680E-47AD-8277-66B3AC01C659}");
            REFLECT_MATH_LIBRARY_TYPE(Color, "{6CC7B2F9-F551-4CB8-A713-4149959AE337}");
            REFLECT_MATH_LIBRARY_TYPE(CRC, "{D8B65A5B-38FF-4B41-9FA4-FCA080D75625}");
            REFLECT_MATH_LIBRARY_TYPE(Matrix3x3, "{8C5F6959-C2C4-46D9-9FCD-4DC234E7732D}");
            REFLECT_MATH_LIBRARY_TYPE(Matrix4x4, "{537AB179-E23C-492D-8EF7-53845A2DB163}");
            REFLECT_MATH_LIBRARY_TYPE(OBB, "{8CBDA3B7-9DCD-4A04-A12C-123C322CA63A}");
            REFLECT_MATH_LIBRARY_TYPE(Plane, "{F2C799DF-2CC1-4DD8-91DC-18C2D517BAB0}");
            REFLECT_MATH_LIBRARY_TYPE(Quaternion, "{9BE75E2E-AA07-4767-9BF3-905C289EB38A}");
            REFLECT_MATH_LIBRARY_TYPE(Random, "{D9DF1385-6C5C-4E41-94CA-8B10E9D8FAAF}");
            REFLECT_MATH_LIBRARY_TYPE(Transform, "{59E0EF87-352C-4CFB-A810-5B8752BDD1EF}");
            REFLECT_MATH_LIBRARY_TYPE(Vector2, "{FD1BFADF-BA1F-4AFC-819B-ABA1F22AE6E6}");
            REFLECT_MATH_LIBRARY_TYPE(Vector3, "{53EA7604-E3AE-4E88-BE0E-66CBC488A7DB}");
            REFLECT_MATH_LIBRARY_TYPE(Vector4, "{66027D7A-FCD1-4592-93E5-EB4B7F4BD671}");
        }
    }

    namespace Library
    {
        void Math::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<Math, LibraryDefinition>()
                    ->Version(1)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<Math>("Math", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Libraries/Math.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".math")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "MathNodeTitlePalette")
                        ;
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
            {
                using namespace ScriptCanvas::Nodes::Math;
                SCRIPT_CANVAS_GENERICS_TO_VM(AABBNodes::Registrar, AABB, behaviorContext, AABBNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(ColorNodes::Registrar, Color, behaviorContext, ColorNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(CRCNodes::Registrar, CRC, behaviorContext, CRCNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(MathNodes::Registrar, Math, behaviorContext, MathNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(Matrix3x3Nodes::Registrar, Matrix3x3, behaviorContext, Matrix3x3Nodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(Matrix4x4Nodes::Registrar, Matrix4x4, behaviorContext, Matrix4x4Nodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(OBBNodes::Registrar, OBB, behaviorContext, OBBNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(PlaneNodes::Registrar, Plane, behaviorContext, PlaneNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(QuaternionNodes::Registrar, Quaternion, behaviorContext, QuaternionNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(RandomNodes::Registrar, Random, behaviorContext, RandomNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(TransformNodes::Registrar, Transform, behaviorContext, TransformNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(Vector2Nodes::Registrar, Vector2, behaviorContext, Vector2Nodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(Vector3Nodes::Registrar, Vector3, behaviorContext, Vector3Nodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(Vector4Nodes::Registrar, Vector4, behaviorContext, Vector4Nodes::k_categoryName);
            }

            Nodes::Internal::ExpressionNodeBase::Reflect(reflection);
        }

        void Math::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Math;

            AddNodeToRegistry<Math, Divide>(nodeRegistry);
            AddNodeToRegistry<Math, MathExpression>(nodeRegistry);
            AddNodeToRegistry<Math, Multiply>(nodeRegistry);
            AddNodeToRegistry<Math, Subtract>(nodeRegistry);
            AddNodeToRegistry<Math, Sum>(nodeRegistry);

            AABBNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            ColorNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            CRCNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            MathNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            Matrix3x3Nodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            Matrix4x4Nodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            OBBNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            PlaneNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            QuaternionNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            RandomNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            TransformNodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            Vector2Nodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            Vector3Nodes::Registrar::AddToRegistry<Math>(nodeRegistry);
            Vector4Nodes::Registrar::AddToRegistry<Math>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Math::GetComponentDescriptors()
        {
            AZStd::vector<AZ::ComponentDescriptor*> descriptors = {
                ScriptCanvas::Nodes::Math::Divide::CreateDescriptor(),
                ScriptCanvas::Nodes::Math::MathExpression::CreateDescriptor(),
                ScriptCanvas::Nodes::Math::Multiply::CreateDescriptor(),
                ScriptCanvas::Nodes::Math::Subtract::CreateDescriptor(),
                ScriptCanvas::Nodes::Math::Sum::CreateDescriptor(),
            };

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
    }
}
