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

#include <Include/ScriptCanvas/Internal/Nodes/StringFormatted.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Internal
        {
            //! This node is intended as a base-class for any node that requires the string formatted capabilities
            //! of generating slots based on curly bracket formatted text to produce input slots.
            class StringFormatted
                : public Node
                , public NodePropertyInterfaceListener
            {
            public:

                SCRIPTCANVAS_NODE(StringFormatted
                );

                using ArrayBindingMap = AZStd::map<AZ::u64, SlotId>;
                using NamedSlotIdMap = AZStd::map<AZStd::string, SlotId>;

                bool ConvertsInputToStrings() const override
                {
                    return true;
                }

                AZ::Outcome<DependencyReport, void> GetDependencies() const override
                {
                    return AZ::Success(DependencyReport{});
                }

                AZ_INLINE Data::StringType GetRawString() const { return *m_stringInterface.GetPropertyData(); }
                AZ_INLINE const NamedSlotIdMap& GetNamedSlotIdMap() const { return m_formatSlotMap; }
                AZ_INLINE const int GetPostDecimalPrecision() const { return m_numericPrecision; }

            protected:

                // This is a map that binds the index into m_unresolvedString to the SlotId that needs to be checked for a valid datum.
                ArrayBindingMap m_arrayBindingMap;

                // A vector of strings that holds all the parts of the string and reserves empty strings for those parts of the string whose
                // values come from slots.
                AZStd::vector<AZStd::string> m_unresolvedString;

                // Maps the slot name to the created SlotId for that slot
                NamedSlotIdMap m_formatSlotMap;

                // Number of precision elements to display
                int m_numericPrecision = 4;

                // The string formatting string used on the node, any value within brackets creates an input slot
                AZStd::string m_format = "{Value}";

                // Node
                void OnInit() override;
                void OnConfigured() override;
                void OnDeserialized() override;
                void ConfigureVisualExtensions() override;

                bool CanDeleteSlot(const SlotId& slotId) const override;

                SlotId HandleExtension(AZ::Crc32 extensionId) override;
                NodePropertyInterface* GetPropertyInterface(AZ::Crc32 propertyId) override;

                void OnSlotRemoved(const SlotId& slotId) override;
                ////

                // Parses the format field to produce the intermediate data for strings that use curly brackets to produce slots.
                void ParseFormat();

                void RelayoutNode();

                // Called when a change to the format string is detected.
                void OnFormatChanged();

                //! Parses the user specified format and resolves the data from the appropriate slots.
                //! \return AZStd::string The fully formatted and resolved string ready for output.
                AZStd::string ProcessFormat();

                AZStd::string GetDisplayGroup() const { return "PrintDisplayGroup"; }
                AZ::Crc32 GetDisplayGroupId() const { return AZ_CRC("PrintDisplayGroup", 0x3c802873); }

                AZ::Crc32 GetExtensionId() const { return AZ_CRC("AddPrintOperand", 0x7aec4eae); }
                AZ::Crc32 GetPropertyId() const { return AZ_CRC("FormatStringProperty", 0x2c587efa); }

                const AZStd::regex& GetRegex() const
                {
                    static AZStd::regex formatRegex(R"(\{(\w+)\})");
                    return formatRegex;
                }

            private:

                // NodePropertyInterface...
                void OnPropertyChanged() override;
                ////

                TypedNodePropertyInterface<ScriptCanvas::Data::StringType> m_stringInterface;
                bool m_parsingFormat = false;

                bool m_isHandlingExtension = false;
            };
        }
    }
}
