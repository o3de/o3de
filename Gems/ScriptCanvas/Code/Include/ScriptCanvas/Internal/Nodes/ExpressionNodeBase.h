/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/string/regex.h>

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

                SCRIPTCANVAS_NODE(ExpressionNodeBase);

                ExpressionNodeBase();

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                AZStd::string GetRawFormat() const;

                const AZStd::unordered_map<AZStd::string, SlotId>& GetSlotsByName() const;
                                
            protected:

                // Maps the slot name to the created SlotId for that slot
                using NamedSlotIdMap = AZStd::map<AZ::Crc32, SlotId>;
                NamedSlotIdMap m_formatSlotMap;

                // The string formatting string used on the node, any value within brackets creates an input slot
                AZStd::string m_format = "";

                // Node...
                void OnInit() override;  
                void OnPostActivate() override;
                void ConfigureVisualExtensions() override;
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
                AZ::Crc32 GetDisplayGroupId() const { return AZ_CRC_CE("ExpressionDisplayGroup"); }

                AZ::Crc32 GetExtensionId() const { return AZ_CRC_CE("AddExpressionOperand"); }
                AZ::Crc32 GetPropertyId() const { return AZ_CRC_CE("FormatStringProperty"); }
                
            protected:
                virtual ExpressionEvaluation::ParseOutcome ParseExpression(const AZStd::string& formatString);

                virtual AZStd::string GetExpressionSeparator() const;

            private:
                void PushVariable(const AZStd::string& variableName, const Datum& datum);

                // NodePropertyInterface...
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
                
                ExpressionEvaluation::ExpressionTree m_expressionTree;

                bool m_isInError = false;

                AZStd::unordered_map<SlotId, AZStd::string> m_slotToVariableMap;
                AZStd::unordered_map<AZStd::string, SlotId> m_slotsByVariables;

                ExpressionEvaluation::ParsingError m_parseError;

                TypedNodePropertyInterface<ScriptCanvas::Data::StringType> m_stringInterface;
                bool m_parsingFormat = false;

                bool m_handlingExtension = false; 
            };
        }
    }
}
