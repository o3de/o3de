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
        }

        void Operators::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            AddNodeToRegistry<Operators, ScriptCanvas::Nodes::NodeableNodeOverloadedLerp>(nodeRegistry, "LerpNodeable");
        }

        AZStd::vector<AZ::ComponentDescriptor*> Operators::GetComponentDescriptors()
        {
            AZStd::vector<AZ::ComponentDescriptor*> descriptors =
            {
                ScriptCanvas::Nodes::NodeableNodeOverloadedLerp::CreateDescriptor(),
            };

            return descriptors;
        }
    }
}
