/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        // Enum to track the different execution phases of the Pass System
        enum class PassSystemState : u32
        {
            // Default state, 
            Unitialized,

            // Initial Pass System setup. Transitions to Idle
            InitializingPassSystem,

            // Pass System is processing passes queued for Removal. Transitions to Idle
            RemovingPasses,

            // Pass System is processing passes queued for Build (and their child passes). Transitions to Idle
            BuildingPasses,

            // Pass System is processing passes queued for Initialization (and their child passes). Transitions to Idle
            InitializingPasses,

            // Pass System is validating that the Pass hierarchy is in a valid state after Build and Initialization. Transitions to Idle
            ValidatingPasses,

            // Pass System is idle and can transition to any other state (except FrameEnd)
            Idle,

            // Pass System is currently rendering a frame. Transitions to FrameEnd
            Rendering,

            // Pass System is finishing rendering a frame. Transitions to Idle
            FrameEnd,
        };

        //! Frame counters used for collecting statistics
        struct PassSystemFrameStatistics
        {
            u32 m_numRenderPassesExecuted = 0;
            u32 m_totalDrawItemsRendered = 0;
            u32 m_maxDrawItemsRenderedInAPass = 0;
        };

        
        enum PassFilterExecutionFlow : uint8_t
        {
            StopVisitingPasses,
            ContinueVisitingPasses,
        };

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
            //! This function should be called before the render pipelines which use templates from this mappings are created.
            //! To load pass template mapping before any render pipelines are created, use OnReadyLoadTemplatesEvent::Handler to
            //! load desired pass template mappings
            virtual bool LoadPassTemplateMappings(const AZStd::string& templateMappingPath) = 0;
            
            //! Writes a pass template to a .pass file which can then be used as a pass asset. Useful for
            //! quickly authoring a pass template in code and then outputting it as a pass asset using JSON
            virtual void WriteTemplateToFile(const PassTemplate& passTemplate, AZStd::string_view assetFilePath) = 0;

            //! Prints the entire pass hierarchy from the root
            virtual void DebugPrintPassHierarchy() = 0;

            //! Returns whether the Pass System is currently hot reloading
            virtual bool IsHotReloading() const = 0;

            //! Sets whether the Pass System is currently hot reloading
            virtual void SetHotReloading(bool hotReloading) = 0;

            //! The pass system enables targeted debugging of a specific pass given the name of the pass
            //! These are the setters and getters for the specific Pass's name
            //! To break in your pass code for a specified pass name, use the macro below
            virtual void SetTargetedPassDebuggingName(const AZ::Name& targetPassName) = 0;
            virtual const AZ::Name& GetTargetedPassDebuggingName() const = 0;

            //! Find the SwapChainPass associated with window Handle
            virtual SwapChainPass* FindSwapChainPass(AzFramework::NativeWindowHandle windowHandle) const = 0;

            using OnReadyLoadTemplatesEvent = AZ::Event<>;
            //! Connect a handler to listen to the event that the pass system is ready to load pass templates
            //! The event is triggered when pass system is initialized and asset system is ready.
            //! The handler can add new pass templates or load pass template mappings from assets
            virtual void ConnectEvent(OnReadyLoadTemplatesEvent::Handler& handler) = 0;

            virtual PassSystemState GetState() const = 0;

            // Passes call this function to notify the pass system that they are drawing X draw items this frame
            // Used for Pass System statistics
            virtual void IncrementFrameDrawItemCount(u32 numDrawItems) = 0;

            // Increments the counter for the number of render passes executed this frame (does not include passes that are disabled) 
            virtual void IncrementFrameRenderPassCount() = 0;

            // Get frame statistics from the Pass System
            virtual PassSystemFrameStatistics GetFrameStatistics() = 0;

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

            //! Adds a PassTemplate to the library
            virtual bool AddPassTemplate(const Name& name, const AZStd::shared_ptr<PassTemplate>& passTemplate) = 0;

            //! Retrieves a PassTemplate from the library
            virtual const AZStd::shared_ptr<PassTemplate> GetPassTemplate(const Name& name) const = 0;

            //! See remarks in PassLibrary.h for the function with this name.
            virtual void RemovePassTemplate(const Name& name) = 0;

            //! Removes all references to the given pass from the pass library
            virtual void RemovePassFromLibrary(Pass* pass) = 0;
                        
            //! Visit the matching passes from registered passes with specified filter
            //! The return value of the passFunction decides if the search continues or not
            //! Note: this function will find all the passes which match the pass filter even they are for render pipelines which are not added to a scene
            //! This function is fast if a pass name or a pass template name is specified. 
            virtual void ForEachPass(const PassFilter& filter, AZStd::function<PassFilterExecutionFlow(Pass*)> passFunction) = 0;

            //! Find the first matching pass from registered passes with specified filter
            //! Note: this function SHOULD ONLY be used when you are certain you only need to handle the first pass found
            virtual Pass* FindFirstPass(const PassFilter& filter) = 0;

        private:
            // These functions are only meant to be used by the Pass class

            // Schedules a pass to have it's Build() function called during frame update
            virtual void QueueForBuild(Pass* pass) = 0;

            // Schedules a pass to be deleted during frame update
            virtual void QueueForRemoval(Pass* pass) = 0;

            // Schedules a pass to be initialized during frame update
            virtual void QueueForInitialization(Pass* pass) = 0;

            //! Registers the pass with the pass library. Called in the Pass constructor.
            virtual void RegisterPass(Pass* pass) = 0;

            //! Unregisters the pass with the pass library. Called in the Pass destructor.
            virtual void UnregisterPass(Pass* pass) = 0;
        };
                
        namespace PassSystemEvents
        {
        }

    }   // namespace RPI
}   // namespace AZ
