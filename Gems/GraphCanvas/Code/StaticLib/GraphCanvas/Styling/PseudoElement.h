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

#include <AzCore/Component/Component.h>

#include <functional>

#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/Selector.h>

namespace GraphCanvas
{
    namespace Styling
    {

        //! Requests that enable virtual styled entities to be created from other styled entities.
        class PseudoElementFactoryRequests : public AZ::EBusTraits
        {
        public:
            //! Creates a standalone style element
            virtual AZ::EntityId CreateStyleEntity(const AZStd::string& style) const = 0;

            //! Create a fake "child element" of a styled entity.
            //! For example, you might have a node, and want to add a "help" element (i.e. "node > help").
            virtual AZ::EntityId CreateVirtualChild(const AZ::EntityId&, const AZStd::string&) const = 0;
        };

        using PseudoElementFactoryRequestBus = AZ::EBus<PseudoElementFactoryRequests>;

        //! A virtual entity that appears to be hierarchically nested below the real styled entity.
        //! For example, you might have a node, and want to add a "help" element (i.e. "node > help").
        //!
        //! Note that the virtual element has the same set of selectors as its parent (with the exception of the
        //! "element" selector), so it will track the parent's state.
        class VirtualChildElement
            : public AZ::Component
            , protected StyledEntityRequestBus::Handler
        {
        public:
            AZ_COMPONENT(VirtualChildElement, "{42C2CC3B-CEEC-434C-916E-3ACED53F7751}");

            static void Reflect(AZ::ReflectContext* context);

            static AZ::EntityId Create(const AZ::EntityId& real, const AZStd::string& virtualChildElement);

            VirtualChildElement();
            virtual ~VirtualChildElement() = default;

        protected:
            VirtualChildElement(const AZ::EntityId& real, const AZStd::string& virtualChildElement);
            VirtualChildElement& operator=(const VirtualChildElement&) = delete;

            // StyledEntityRequestBus::Handler
            AZ::EntityId GetStyleParent() const override;
            SelectorVector GetStyleSelectors() const override;

            void AddSelectorState(const char* selector) override;
            void RemoveSelectorState(const char* selector) override;

            AZStd::string GetElement() const override;
            AZStd::string GetClass() const override;
            ////

            // AZ::Component
            void Activate() override;
            void Deactivate() override;
            ////

            AZ::EntityId m_real;

            Selector m_parentSelector;
            AZStd::string m_virtualChild;
            Selector m_virtualChildSelector;

            AZStd::unordered_map<AZ::Crc32, Styling::Selector> m_dynamicSelectors;
        };

    } // namespace Styling
} // namespace GraphCanvas
