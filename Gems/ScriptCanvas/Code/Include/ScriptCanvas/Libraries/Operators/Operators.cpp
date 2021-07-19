/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Libraries/Libraries.h>
#include "Operators.h"
#include <ScriptCanvas/Core/Attributes.h>
#include <Libraries/Operators/Math/OperatorLerpNodeableNode.h>

namespace ScriptCanvas
{
    namespace Library
    {
        void Operators::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Operators, LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Operators>("Operators", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Libraries/Operators.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".operators")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "OperatorsNodeTitlePalette")
                        ;
                }
            }

            ScriptCanvas::Nodes::LerpBetweenNodeable<float>::Reflect(reflection);
            ScriptCanvas::Nodes::LerpBetweenNodeable<Data::Vector2Type>::Reflect(reflection);
            ScriptCanvas::Nodes::LerpBetweenNodeable<Data::Vector3Type>::Reflect(reflection);
            ScriptCanvas::Nodes::LerpBetweenNodeable<Data::Vector4Type>::Reflect(reflection);
            ScriptCanvas::Nodes::NodeableNodeOverloaded::Reflect(reflection);

            Nodes::Operators::OperatorBase::Reflect(reflection);
            Nodes::Operators::OperatorArithmetic::Reflect(reflection);
            Nodes::Operators::OperatorArithmeticUnary::Reflect(reflection);
        }

        void Operators::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Operators;

            // Math
            AddNodeToRegistry<Operators, OperatorAdd>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorDiv>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorMul>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorSub>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorLength>(nodeRegistry);
            AddNodeToRegistry<Operators, Nodes::LerpBetween>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorDivideByNumber>(nodeRegistry);
            AddNodeToRegistry<Operators, ScriptCanvas::Nodes::NodeableNodeOverloadedLerp>(nodeRegistry, "LerpNodeable");

            // Containers
            AddNodeToRegistry<Operators, OperatorAt>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorBack>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorClear>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorErase>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorFront>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorInsert>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorEmpty>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorSize>(nodeRegistry);
            AddNodeToRegistry<Operators, OperatorPushBack>(nodeRegistry);

        }

        AZStd::vector<AZ::ComponentDescriptor*> Operators::GetComponentDescriptors()
        {
            AZStd::vector<AZ::ComponentDescriptor*> descriptors =
            {
                // Math
                ScriptCanvas::Nodes::Operators::OperatorAdd::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorDiv::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorMul::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorSub::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorLength::CreateDescriptor(),
                ScriptCanvas::Nodes::LerpBetween::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorDivideByNumber::CreateDescriptor(),
                ScriptCanvas::Nodes::NodeableNodeOverloadedLerp::CreateDescriptor(),

                // Containers
                ScriptCanvas::Nodes::Operators::OperatorAt::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorBack::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorClear::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorErase::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorFront::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorInsert::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorEmpty::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorSize::CreateDescriptor(),
                ScriptCanvas::Nodes::Operators::OperatorPushBack::CreateDescriptor(),
            };

            return descriptors;
        }
    }
}
