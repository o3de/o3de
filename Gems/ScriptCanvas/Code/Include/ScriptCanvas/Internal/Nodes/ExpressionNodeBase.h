/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/regex.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Core/Node.h>

#include <ExpressionEvaluation/ExpressionEngine.h>

#include <Include/ScriptCanvas/Internal/Nodes/ExpressionNodeBase.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Internal
        {
            //! Base class that will handle most of the visual and configuration for Expression based nodes.
            class ExpressionNodeBase
                : public Node
                , public NodePropertyInterfaceListener
            {
            public:
                ScriptCanvas_Node(ExpressionNodeBase,
                    ScriptCanvas_Node::Name("ExpressionNodeBase", "Base class for any node that wants to evaluate user given expressions.")
                    ScriptCanvas_Node::Uuid("{797C800D-8C96-4675-B5B5-2A321AC09433}")
                    ScriptCanvas_Node::Category("Internal")
                    ScriptCanvas_Node::DynamicSlotOrdering(true)
                    ScriptCanvas_Node::Version(0)
                );

                ExpressionNodeBase();

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                ScriptCanvas_EditPropertyWithDefaults(AZStd::string, m_format, "",
                    EditProperty::Name("Expression", "The expression string; Any word within {} will create a data pin on the node.")
                    EditProperty::EditAttributes(AZ::Edit::Attributes::StringLineEditingCompleteNotify(&ExpressionNodeBase::SignalFormatChanged)
                    )
                );

                // Maps the slot name to the created SlotId for that slot
                using NamedSlotIdMap = AZStd::map<AZ::Crc32, SlotId>;
                NamedSlotIdMap m_formatSlotMap;

                // Node
                void OnInit() override;  
                void OnPostActivate() override;
                void ConfigureVisualExtensions() override;

                void OnInputSignal(const SlotId& slotId) override;
                void OnInputChanged(const Datum& input, const SlotId& slotId) override;

                bool CanDeleteSlot(const SlotId& slotId) const override;

                SlotId HandleExtension(AZ::Crc32 extensionId) override;
                void ExtensionCancelled(AZ::Crc32 extensionId) override;
                void FinalizeExtension(AZ::Crc32 extensionId) override;
                NodePropertyInterface* GetPropertyInterface(AZ::Crc32 propertyId) override;

                void OnSlotRemoved(const SlotId& slotId) override;

                bool OnValidateNode(ValidationResults& validationResults) override;
                ////

                void ParseFormat(bool signalError = true);

                void SignalFormatChanged();

                AZStd::string GetDisplayGroup() const { return "ExpressionDisplayGroup"; }
                AZ::Crc32 GetDisplayGroupId() const { return AZ_CRC("ExpressionDisplayGroup", 0x770de38e); }

                AZ::Crc32 GetExtensionId() const { return AZ_CRC("AddExpressionOperand", 0x5f5fdcab); }
                AZ::Crc32 GetPropertyId() const { return AZ_CRC("FormatStringProperty", 0x2c587efa); }
                
            protected:
            
                virtual void OnResult(const ExpressionEvaluation::ExpressionResult& result);
                virtual ExpressionEvaluation::ParseOutcome ParseExpression(const AZStd::string& formatString);

                virtual AZStd::string GetExpressionSeparator() const;

            private:

                void PushVariable(const AZStd::string& variableName, const Datum& datum);

                // NodePropertyInterface
                void OnPropertyChanged() override;
                ////

                void ConfigureSlot(const AZStd::string& variableName, SlotConfiguration& slotConfiguration);

                struct SlotCacheSetup
                {
                    SlotId m_previousId;
                    Data::Type m_displayType;

                    VariableId m_reference;
                    Datum      m_defaultValue;
                };
                
                ScriptCanvas_SerializeProperty(ExpressionEvaluation::ExpressionTree, m_expressionTree);

                // Going to store out a bool here to track whether or not we have an error. This way I don't need to parse everything all the time
                // And I don't need to pointlessly store a potentially long error message in the save data.
                ScriptCanvas_SerializePropertyWithDefaults(bool, m_isInError, false);

                AZStd::unordered_map<SlotId, AZStd::string> m_slotToVariableMap;
                AZStd::unordered_set<SlotId> m_dirtyInputs;

                ExpressionEvaluation::ParsingError m_parseError;

                TypedNodePropertyInterface<ScriptCanvas::Data::StringType> m_stringInterface;
                bool m_parsingFormat = false;

                bool m_handlingExtension = false; 
            };
        }
    }
}
