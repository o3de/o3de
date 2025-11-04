#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <limits>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }
        namespace DataTypes
        {
            class IManifestObject;
        }
        namespace Events
        {
            class SCENE_CORE_API ManifestMetaInfo
                : public AZ::EBusTraits
            {
            public:
                static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

                struct CategoryRegistration
                {
                    AZStd::string m_categoryName;
                    AZ::Uuid m_categoryTargetGroupId;
                    int m_preferredOrder;

                    CategoryRegistration(const char* categoryName, const AZ::Uuid& categoryTargetId, int preferredOrder = std::numeric_limits<int>::max())
                        : m_categoryName(categoryName)
                        , m_categoryTargetGroupId(categoryTargetId)
                        , m_preferredOrder(preferredOrder)
                    {
                    }
                };
                using CategoryRegistrationList = AZStd::vector<CategoryRegistration>;
                using ModifiersList = AZStd::vector<AZ::Uuid>;

                ManifestMetaInfo();
                virtual ~ManifestMetaInfo() = 0;

                //! Gets a list of all the categories and the class identifiers that are listed for that category.
                virtual void GetCategoryAssignments(CategoryRegistrationList& categories, const Containers::Scene& scene);

                //! Gets the path to the icon associated with the given object.
                virtual void GetIconPath(AZStd::string& iconPath, const DataTypes::IManifestObject& target);

                //! Gets a list of a the modifiers (such as rules for  groups) that the target accepts.
                //! Note that updates to the target may change what modifiers can be accepted. For instance
                //! if a group only accepts a single rule of a particular type, calling this function a second time
                //! will not include the uuid of that rule.
                //! This method is called when the "Add Modifier" button is pressed in the FBX Settings Editor.
                virtual void GetAvailableModifiers(ModifiersList& modifiers, const Containers::Scene& scene,
                    const DataTypes::IManifestObject& target);

                //! Initialized the given manifest object based on the scene. Depending on what other entries have been added
                //! to the manifest, an implementation of this function may decided that certain values should or shouldn't
                //! be added, such as not adding meshes to a group that already belong to another group.
                //! This method is always called each time a Group type of object is created in memory (e.g. When the user
                //! clicks the "Add another Mesh" or "Add another Actor" in the FBX Settings Editor). Overriders of this method
                //! should check the type of the \p target to decide to take action (e.g. add a Modifier) or do nothing.
                virtual void InitializeObject(const Containers::Scene& scene, DataTypes::IManifestObject& target);

                //! Called when an existing object is updated. This is not called when an object is initialized, which is handled,
                //! by InitializeObject, but a parent may still get the update. For instance adding or removing a rule will
                //! have this called for the parent group.
                //! @param scene The scene the object belongs to.
                //! @param target The object that's being updated. If this is null it refers to an update to the entire manifest, for
                //! when a group is deleted for instance.
                //! @param sender An optional argument to keep track of the object that called this function. This can be used if the
                //! same object that sends a message also handles the callback to avoid recursively updating.
                virtual void ObjectUpdated(const Containers::Scene& scene, const DataTypes::IManifestObject* target, void* sender = nullptr);

                //! Manifest management is two phases: The UI for editing scene settings tends to work in the manifest objects directly,
                //! updating the actual scene. If the scene is directly edited as a response to InitializeObject or ObjectUpdated
                //! (with the scene made non-const), then the UI won't actually refresh, because it's operating on stale data.
                //! The intended flow here is, if a listener on this bus wants to add aditional objects to the scene manifest in the UI:
                //! 1) Listen to the InitializeObject or ObjectUpdated command. 2) Create the vector of new manifest objects that should
                //! be created in response to that command. 3) Emit this message, so the UI can respond and update/add those objects.
                //! This shouldn't be called during asset processing, it won't be as functional.
                virtual void AddObjects([[maybe_unused]] AZStd::vector<AZStd::shared_ptr<DataTypes::IManifestObject>>& objects) {}
            };

            inline ManifestMetaInfo::~ManifestMetaInfo() = default;

            using ManifestMetaInfoBus = AZ::EBus<ManifestMetaInfo>;
        } // namespace Events
    } // namespace SceneAPI
} // namespace AZ
