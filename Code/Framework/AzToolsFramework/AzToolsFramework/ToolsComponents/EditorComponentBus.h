/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
 * @file
 * Header file for the editor component base class.
 * Derive from this class to create a version of a component to use in the
 * editor, as opposed to the version of the component that is used during run time.
 *
 * To learn more about editor components, see the
 * [Editor Components documentation](https://www.o3de.org/docs/user-guide/programming/components/editor-components/).
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>


namespace AzToolsFramework
{
    namespace Components
    {        
        /// @cond EXCLUDE_DOCS

        /**
         * Interface for AzToolsFramework::Components::EditorComponentDescriptorBus,
         * which handles requests to the editor component regarding editor-only functionality.
         * Do not assume that all editor components have it.
         */
        class EditorComponentDescriptor
        {
        public:

            /**
             * Checks the equality of components.
             *
             * If you want your component to have a custom DoComponentsMatch()
             * function, you need to do the following:
             * - Put the AZ_EDITOR_COMPONENT macro in your class, instead of AZ_COMPONENT.
             * - Define a static DoComponentsMatch() function with the following signature:
             *
             * `bool DoComponentsMatch(const ComponentClass*, const ComponentClass*); // where ComponentClass is the type of the class containing this function.`
             *
             * For example, ScriptComponents have a custom DoComponentsMatch() function
             * so that two ScriptComponents, which are in C++, are determined to be equal
             * only if they use the same Lua file to define their behavior.
             *
             * @param thisComponent The first component to compare.
             * @param otherComponent The component to compare with the first component.
             * @return True if the components are the same. Otherwise, false.
             */
            virtual bool DoComponentsMatch(const AZ::Component* thisComponent, const AZ::Component* otherComponent) const = 0;

            /**
             * Allows a "paste-over" operation on this component.
             *
             * If you want to allow this functionality on your component, you need to
             * do the following:
             * - Put the AZ_EDITOR_COMPONENT macro in your class, instead of AZ_COMPONENT.
             * - Define a static PasteOverComponent() function with the following signature:
             *
             * `void PasteOverComponent(const ComponentClass* sourceComponent, ComponentClass* destinationComponent); // where ComponentClass is the type of the class containing this function.`
             *
             * @param sourceComponent The component to pull data from.
             * @param destinationComponent The component to apply the paste operation to.
             */
            virtual void PasteOverComponent(const AZ::Component* sourceComponent, AZ::Component* destinationComponent) = 0;

            /**
             * Checks if "paste-over" is supported on this component.
             *
             * @return True if the component this is describing implements PasteOverComponent.
             */
            virtual bool SupportsPasteOver() const = 0;

            /**
             * Returns the editor component descriptor of the current component.
             * @return A pointer to the editor component descriptor.
             */
            virtual EditorComponentDescriptor* GetEditorDescriptor() { return this; }
        };

        /**
         * The properties of the editor component descriptor EBus.
         */
        using EditorComponentDescriptorBusTraits = AZ::ComponentDescriptorBusTraits;

        /**
         * An EBus for requests to the editor component.
         * The events are defined in the AzToolsFramework::Components::EditorComponentDescriptor class.
         */
        using EditorComponentDescriptorBus = AZ::EBus<EditorComponentDescriptor, EditorComponentDescriptorBusTraits>;
    } // namespace Components
} // namespace AzToolsFramework

DECLARE_EBUS_EXTERN_DLL_MULTI_ADDRESS_WITH_TRAITS(AzToolsFramework::Components::EditorComponentDescriptor, AZ::ComponentDescriptorBusTraits);
