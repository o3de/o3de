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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string_view.h>

#include <Atom/RPI.Public/Pass/PassDefines.h>

#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>

#include <AzFramework/Windowing/WindowBus.h>

namespace AZ
{
    namespace RPI
    {
        class Pass;
        class PassAsset;
        struct PassDescriptor;
        class PassFilter;
        class PassLibrary;
        struct PassRequest;
        class PassTemplate;
        class ParentPass;
        class SwapChainPass;

        using PassCreator = AZStd::function<Ptr<Pass>(const PassDescriptor& descriptor)>;

        class PassSystemInterface
        {
            friend class Pass;
            friend class ParentPass;
            friend class PassTests;
        public:
            AZ_RTTI(PassSystemInterface, "{19DE806F-F1B2-4B1E-A0F2-F8BA85B4552E}");

            PassSystemInterface() = default;
            virtual ~PassSystemInterface() = default;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(PassSystemInterface);

            static PassSystemInterface* Get();

            //! Returns the root of the pass hierarchy
            virtual const Ptr<ParentPass>& GetRootPass() = 0;

            //! Processes pass tree changes that were queued by QueueFor*() functions. This is called
            //! automatically in FrameUpdate(), but may be called manually when needed, like when 
            //! initializing a scene;
            virtual void ProcessQueuedChanges() = 0;
            
            //! Load pass templates listed in a name-assetid mapping asset 
            virtual void LoadPassTemplateMappings(const AZStd::string& templateMappingPath) = 0;
            
            //! Writes a pass template to a .pass file which can then be used as a pass asset. Useful for
            //! quickly authoring a pass template in code and then outputting it as a pass asset using JSON
            virtual void WriteTemplateToFile(const PassTemplate& passTemplate, AZStd::string_view assetFilePath) = 0;

            //! Prints the entire pass hierarchy from the root
            virtual void DebugPrintPassHierarchy() = 0;

            //! Returns whether the Pass System is currently in it's build phase
            virtual bool IsBuilding() const = 0;

            //! Returns whether the Pass System is currently hot reloading
            virtual bool IsHotReloading() const = 0;

            //! Sets whether the Pass System is currently hot reloading
            virtual void SetHotReloading(bool hotReloading) = 0;

            //! The pass system enables targeted debugging of a specific pass given the name of the pass
            //! These are the setters and getters for the specific Pass's name
            //! To break in your pass code for a specified pass name, use the macro below
            virtual void SetTargetedPassDebuggingName(const AZ::Name& targetPassName) = 0;
            virtual const AZ::Name& GetTargetedPassDebuggingName() const = 0;

            // --- Pass Factory related functionality ---

            //! Directly creates a pass given a PassDescriptor
            template<typename PassType>
            Ptr<PassType> CreatePass(const PassDescriptor& descriptor)
            {
                Ptr<PassType> pass = PassType::Create(descriptor);
                return AZStd::move(pass);
            }

            //! Directly creates a pass given a Name
            template<typename PassType>
            Ptr<PassType> CreatePass(Name name)
            {
                PassDescriptor passDescriptor(name);
                return CreatePass<PassType>(passDescriptor);
            }

            //! Registers a PassCreator with the PassFactory
            virtual void AddPassCreator(Name className, PassCreator createFunction) = 0;

            //! Creates a Pass using the name of the Pass class
            virtual Ptr<Pass> CreatePassFromClass(Name passClassName, Name passName) = 0;

            //! Creates a Pass using a PassTemplate
            virtual Ptr<Pass> CreatePassFromTemplate(const AZStd::shared_ptr<PassTemplate>& passTemplate, Name passName) = 0;

            //! Creates a Pass using the name of a PassTemplate
            virtual Ptr<Pass> CreatePassFromTemplate(Name templateName, Name passName) = 0;

            //! Creates a Pass using a PassRequest
            virtual Ptr<Pass> CreatePassFromRequest(const PassRequest* passRequest) = 0;

            //! Returns true if the factory has a creator for a given pass class name
            virtual bool HasCreatorForClass(Name passClassName) = 0;

            // --- Pass Library related functionality ---

            //! Returns true if the pass factory contains passes created with the given template name
            virtual bool HasPassesForTemplateName(const Name& templateName) const = 0;

            //! Get the passes created with the given template name.
            virtual const AZStd::vector<Pass*>& GetPassesForTemplateName(const Name& templateName) const = 0;

            //! Adds a PassTemplate to the library
            virtual bool AddPassTemplate(const Name& name, const AZStd::shared_ptr<PassTemplate>& passTemplate) = 0;

            //! Retrieves a PassTemplate from the library
            virtual const AZStd::shared_ptr<PassTemplate> GetPassTemplate(const Name& name) const = 0;

            //! Removes all references to the given pass from the pass library
            virtual void RemovePassFromLibrary(Pass* pass) = 0;

            //! Find matching passes from registered passes with specified filter
            virtual AZStd::vector<Pass*> FindPasses(const PassFilter& passFilter) const = 0;

            //! Find the SwapChainPass associated with window Handle
            virtual SwapChainPass* FindSwapChainPass(AzFramework::NativeWindowHandle windowHandle) const = 0;

        private:
            // These functions are only meant to be used by the Pass class

            // Schedules a pass to have it's BuildAttachments() function called during frame update
            virtual void QueueForBuildAttachments(Pass* pass) = 0;

            // Schedules a pass to be deleted during frame update
            virtual void QueueForRemoval(Pass* pass) = 0;

            //! Registers the pass with the pass library. Called in the Pass constructor.
            virtual void RegisterPass(Pass* pass) = 0;

            //! Unregisters the pass with the pass library. Called in the Pass destructor.
            virtual void UnregisterPass(Pass* pass) = 0;

        };

        //! Notifications of the pass system such attachments were rebuilt, pass tree changes
        class PassSystemNotificiations
            : public AZ::EBusTraits
        {
        public:

            //! Notify when any pass's attachment was rebuilt
            virtual void OnPassAttachmentsBuilt() = 0;
        };

        using PassSystemNotificiationBus = AZ::EBus<PassSystemNotificiations>;
    }   // namespace RPI
}   // namespace AZ
