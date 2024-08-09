/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>

namespace AzToolsFramework
{
    class EntityCompositionRequests
        : public AZ::EBusTraits
    {
    public:

        struct AddComponentsResults
        {
            // This is the original list of components added (whether or not they are pending, in the order of class data requested)
            AZ::Entity::ComponentArrayType m_componentsAdded;
            /*!
            * Adding a component can only cause the following to occur:
            * 1) Component gets added to the pending list
            * 2) Components Gets added to the entity as a valid component
            * 3) Cause pending components to be added to the entity as valid components (by satisfying previously missing services)
            *
            * The following three vectors represent each occurrence, respectively.
            */
            AZ::Entity::ComponentArrayType m_addedPendingComponents;
            AZ::Entity::ComponentArrayType m_addedValidComponents;
            AZ::Entity::ComponentArrayType m_additionalValidatedComponents;
        };

        /*!
         * Stores a map of entity ids to component results that were added during AddComponentsToEntities.
         * You can use this to look up what exactly happened to each entity involved.
         * Components requested to be added will be stored in either addedPendingComponents or addedValidComponents
         * Any other previously pending components that are now valid will be stored in additionalValidatedComponents
         */
        using EntityToAddedComponentsMap = AZStd::unordered_map<AZ::EntityId, AddComponentsResults>;

        /*!
        * Outcome will be true if successful and return the above results structure to indicate what happened
        * Outcome will be false if critical underlying system failure occurred (which is not expected) and an error string will describe the problem
        */
        using AddComponentsOutcome = AZ::Outcome<EntityToAddedComponentsMap, AZStd::string>;

        /*!
        * Outcome will be true if successful and return one instance of the above AddComponentsResults structure (since only one entity is involved)
        */
        using AddExistingComponentsOutcome = AZ::Outcome<AddComponentsResults, AZStd::string>;

        /*!
        * Add the specified component types to the specified entities.
        *
        * \param entityIds Entities to receive the new components.
        * \param componentsToAdd A list of AZ::Uuid representing the unique id of the type of components to add.
        *
        * \return  Returns a successful outcome if components were added to entities.
        *          If the operation could not be completed then the failed
        *          outcome contains a string describing what went wrong.
        */
        virtual AddComponentsOutcome AddComponentsToEntities(const EntityIdList& entityIds, const AZ::ComponentTypeList& componentsToAdd) = 0;

        /*!
        * Add the specified existing components to the specified entity.
        *
        * \param entityId The AZ::EntityId to add the existing components to, with full editor-level checking with pending component support
        * \param componentsToAdd A list of AZ::Component* containing existing components to add. (Note: These components must not already be tied to another entity!)
        *
        * \return  Returns a successful outcome if components were added to entities.
        *          If the operation could not be completed then the failed
        *          outcome contains a string describing what went wrong.
        */
        virtual AddExistingComponentsOutcome AddExistingComponentsToEntityById(const AZ::EntityId& entityId, AZStd::span<AZ::Component* const> componentsToAdd) = 0;

        // Removing a component can only cause the following to occur:
        // 1) Invalidate other components by removing missing services
        // 2) Validate other components by removing conflicting pending services
        struct RemoveComponentsResults
        {
            //! Invalidated Components are those that were previously valid but no longer are valid.
            AZ::Entity::ComponentArrayType m_invalidatedComponents;
            
            //! Validated Components are those that were previously invalid (and being held back) and are now valid.
            //! Note that during a "Scrub" operation, this remains true - it will only list those that were previously
            //! inactive, and were activated by the scrub, it will not contain the list of previously active that remain active.
            AZ::Entity::ComponentArrayType m_validatedComponents;
        };
        using EntityToRemoveComponentsResultMap = AZStd::unordered_map<AZ::EntityId, RemoveComponentsResults>;
        using RemoveComponentsOutcome = AZ::Outcome<EntityToRemoveComponentsResultMap, AZStd::string>;
        /*!
        * Removes the specified components from the specified entities.
        *
        * \param componentsToRemove List of component pointers to remove (from their respective entities).
        * \return true if the components were successfully removed or false otherwise.
        */
        virtual RemoveComponentsOutcome RemoveComponents(AZStd::span<AZ::Component* const> componentsToRemove) = 0;

        using ScrubEntityResults = RemoveComponentsResults;
        using EntityToScrubEntityResultsMap = AZStd::unordered_map<AZ::EntityId, ScrubEntityResults>;
        using ScrubEntitiesOutcome = AZ::Outcome<EntityToScrubEntityResultsMap, AZStd::string>;

        /*!
         * Scrub entities so that they can be activated.
         * Components will be moved to the pending list if they cannot be activated.
         * If a component had been pending, but can now be activated, then it will be re-enabled.
         *
         * ScrubEntities() may be called on entities before they are initialized.
         *
         * \return If successful, outcome contains details about the scrubbing.
         * If unsuccessful, outcome contains a string describing what went wrong.
         * 
         * To decipher the outcome, understand that when you run the scrub an entity, 4 possible things can happen to each component:
         *      1) Component was active before and remains active now 
         *          --> These can be retrieved from Entity::GetComponents() and are unchanged
         *      2) Component was active before, but is now INACTIVE due to invalid requirements.
         *          --> These are on the Outcome's m_invalidatedComponents list.
         *          --> They are also added to a EditorPendingCompositionComponent on the entity.  This component's job is to keep track
         *              of invalid components and save their data in case they become active again in a later scrub.
         *      3) Components which were inactive before (because of #2 above, in a previous scrub), but now have their requirements
         *         satisfied during this new scrub.
         *          --> These will be in Entity::GetComponents() but also the m_validated components list to distinguish them from the first case 1) above.
         *      4) "Hidden" built-in components may be deprecated or invalid.  
         *         ---> These will be deleted and a warning will be issued.  They will not be in any list, since they are deleted.
         */
        virtual ScrubEntitiesOutcome ScrubEntities(const EntityList& entities) = 0;

        /*!
        * Removes the given components from their respective entities (currently only single entity is supported) and copies the data to the clipboard if successful
        * \param components vector of components to cut (this method will delete the components provided on successful removal)
        */
        virtual void CutComponents(AZStd::span<AZ::Component* const> components) = 0;

        /*!
        * Copies the given components from their respective entities (multiple source entities are supported) into mime data on the clipboard for pasting elsewhere
        * \param components vector of components to copy
        */
        virtual void CopyComponents(AZStd::span<AZ::Component* const> components) = 0;

        /*!
        * Pastes components from the mime data on the clipboard (assuming it is component data) to the given entity
        * \param entityId the Id of the entity to paste to
        */
        virtual void PasteComponentsToEntity(AZ::EntityId entityId) = 0;

        /*!
        * Checks if there is component data available to paste into an entity
        * \return true if paste is available, false otherwise
        */
        virtual bool HasComponentsToPaste() = 0;

        /*!
        * Enables the given components
        * \param components vector of components to enable
        */
        virtual void EnableComponents(AZStd::span<AZ::Component* const> components) = 0;

        /*!
        * Disables the given components
        * \param components vector of components to disable
        */
        virtual void DisableComponents(AZStd::span<AZ::Component* const> components) = 0;


        /*!
         * Info detailing why a pending component cannot be activated.
         */
        struct PendingComponentInfo
        {
            AZ::Entity::ComponentArrayType m_validComponentsThatAreIncompatible;
            AZ::Entity::ComponentArrayType m_pendingComponentsWithRequiredServices;
            AZ::ComponentDescriptor::StringWarningArray m_warnings;
            AZ::ComponentDescriptor::DependencyArrayType m_missingRequiredServices;
            AZ::ComponentDescriptor::DependencyArrayType m_incompatibleServices;
        };

        /*
         * Returns detailed info regarding a pending component.
         * Pending components are those that cannot be activated due to
         * missing requirements or incompatibilities with another component.
         */
        virtual PendingComponentInfo GetPendingComponentInfo(const AZ::Component* component) = 0;

        /*!
        * Returns a name for the given component Note: This will always dig into the underlying type. e.g. you will never get the GenericComponentWrapper name, but always the actual underlying component
        * \param component the pointer to the component for which you want the name.
        */
        virtual AZStd::string GetComponentName(const AZ::Component* component) = 0;
    };

    using EntityCompositionRequestBus = AZ::EBus<EntityCompositionRequests>;

    //! Return whether component should appear in an entity's "Add Component" menu.
    //! \param entityType The type of entity (ex: "Game", "System")
    static bool AppearsInAddComponentMenu(const AZ::SerializeContext::ClassData& classData, const AZ::Crc32& entityType);

    //! ComponentFilter for components that users can add to game entities.
    static bool AppearsInGameComponentMenu(const AZ::SerializeContext::ClassData&);

    //
    // Implementation
    //

    inline bool AppearsInAddComponentMenu(const AZ::SerializeContext::ClassData& classData, const AZ::Crc32& entityType)
    {
        if (classData.m_editData)
        {
            if (auto editorDataElement = classData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                for (const AZ::Edit::AttributePair& attribPair : editorDataElement->m_attributes)
                {
                    if (attribPair.first == AZ::Edit::Attributes::AppearsInAddComponentMenu)
                    {
                        PropertyAttributeReader reader(nullptr, attribPair.second);
                        AZ::Crc32 classEntityType = 0;
                        AZStd::vector<AZ::Crc32> classEntityTypes;

                        if (reader.Read<AZ::Crc32>(classEntityType))
                        {
                            if (static_cast<AZ::u32>(entityType) == classEntityType)
                            {
                                return true;
                            }
                        }
                        else if (reader.Read<AZStd::vector<AZ::Crc32>>(classEntityTypes))
                        {
                            if (AZStd::find(classEntityTypes.begin(), classEntityTypes.end(), entityType) != classEntityTypes.end())
                            {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    inline bool AppearsInGameComponentMenu(const AZ::SerializeContext::ClassData& classData)
    {
        // We don't call AppearsInAddComponentMenu(...) because we support legacy values.
        // AppearsInAddComponentMenu used to be a bool,
        // and it used to only be applied to components on in-game entities.
        if (classData.m_editData)
        {
            if (auto editorDataElement = classData.m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
            {
                for (const AZ::Edit::AttributePair& attribPair : editorDataElement->m_attributes)
                {
                    if (attribPair.first == AZ::Edit::Attributes::AppearsInAddComponentMenu)
                    {
                        PropertyAttributeReader reader(nullptr, attribPair.second);
                        AZ::Crc32 classEntityType;
                        AZStd::vector<AZ::Crc32> classEntityTypes;
                        if (reader.Read<AZ::Crc32>(classEntityType))
                        {
                            if (classEntityType == AZ_CRC_CE("Game"))
                            {
                                return true;
                            }
                        }
                        else if (reader.Read<AZStd::vector<AZ::Crc32>>(classEntityTypes))
                        {
                            if (AZStd::find(classEntityTypes.begin(), classEntityTypes.end(), AZ_CRC_CE("Game")) != classEntityTypes.end())
                            {
                                return true;
                            }
                        }

                        bool legacyAppearsInComponentMenu = false;
                        if (reader.Read<bool>(legacyAppearsInComponentMenu))
                        {
                            AZ_WarningOnce(classData.m_name, false, "%s %s 'AppearsInAddComponentMenu' uses legacy value 'true', should be 'AZ_CRC(\"Game\")'.",
                                classData.m_name, classData.m_typeId.ToString<AZStd::string>().c_str());
                            return legacyAppearsInComponentMenu;
                        }
                    }
                }
            }
        }
        return false;
    }

    inline bool AppearsInLayerComponentMenu(const AZ::SerializeContext::ClassData& classData)
    {
        return AppearsInAddComponentMenu(classData, AZ_CRC_CE("Layer"));
    }

    inline bool AppearsInLevelComponentMenu(const AZ::SerializeContext::ClassData& classData)
    {
        return AppearsInAddComponentMenu(classData, AZ_CRC_CE("Level"));
    }

    inline bool AppearsInAnyComponentMenu(const AZ::SerializeContext::ClassData& classData)
    {
        return (AppearsInGameComponentMenu(classData) || AppearsInLayerComponentMenu(classData) || AppearsInLevelComponentMenu(classData));
    }
}
