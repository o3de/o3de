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

                SCRIPT_CANVAS_GENERICS_TO_VM(MathNodes::Registrar, Math, behaviorContext, MathNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM(RandomNodes::Registrar, Random, behaviorContext, RandomNodes::k_categoryName);

                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(AABBNodes::Registrar, behaviorContext, AABBNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(CRCNodes::Registrar, behaviorContext, CRCNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(ColorNodes::Registrar, behaviorContext, ColorNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(Matrix3x3Nodes::Registrar, behaviorContext, Matrix3x3Nodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(Matrix4x4Nodes::Registrar, behaviorContext, Matrix4x4Nodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(OBBNodes::Registrar, behaviorContext, OBBNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(PlaneNodes::Registrar, behaviorContext, PlaneNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(QuaternionNodes::Registrar, behaviorContext, QuaternionNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(TransformNodes::Registrar, behaviorContext, TransformNodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(Vector2Nodes::Registrar, behaviorContext, Vector2Nodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(Vector3Nodes::Registrar, behaviorContext, Vector3Nodes::k_categoryName);
                SCRIPT_CANVAS_GENERICS_TO_VM_LIBRARY_ONLY(Vector4Nodes::Registrar, behaviorContext, Vector4Nodes::k_categoryName);
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
