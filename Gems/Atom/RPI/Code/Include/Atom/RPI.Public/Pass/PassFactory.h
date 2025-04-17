/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

#include <Atom/RHI.Reflect/Handle.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

namespace AZ
{
    namespace RHI
    {
        class FrameGraphBuilder;
    }


    namespace RPI
    {
        class Pass;
        class PassLibrary;
        class PassTemplate;
        class Scene;

       //! The PassFactory is responsible for creating passes throughout the RPI. It exposes functions for
       //! creating passes using either a Pass Name, a PassTemplate, a PassTemplate Name, or a PassRequest.
       //! To register a new pass type with the PassFactory, write a static create method for your pass class
       //! and register it with the PassFactory using the AddPassCreator function.
        class ATOM_RPI_PUBLIC_API PassFactory final
        {
            friend class PassTests;
            using CreatorIndex = RHI::Handle<uint32_t, PassCreator>;

        public:
            AZ_CLASS_ALLOCATOR(PassFactory, SystemAllocator);

            PassFactory() = default;

            AZ_DISABLE_COPY_MOVE(PassFactory);

            //! Initializes the PassFactory and adds core PassCreators to the PassFactory
            void Init(PassLibrary* passLibrary);

            void Shutdown();

            //! Registers a PassCreator with the PassFactory
            void AddPassCreator(Name passClassName, PassCreator createFunction);

            //! Creates a Pass using the name of the Pass class
            Ptr<Pass> CreatePassFromClass(Name passClassName, Name passName);

            //! Creates a Pass using a PassTemplate
            Ptr<Pass> CreatePassFromTemplate(const AZStd::shared_ptr<const PassTemplate>& passTemplate, Name passName);

            //! Creates a Pass using the name of a PassTemplate
            Ptr<Pass> CreatePassFromTemplate(Name templateName, Name passName);

            //! Creates a Pass using a PassRequest
            Ptr<Pass> CreatePassFromRequest(const PassRequest* passRequest);

            //! Returns true if the factory has a creator for a given pass class name
            bool HasCreatorForClass(Name passClassName);

        private:

            // Registers static create functions for passes core to the RPI with the PassFactory
            void AddCorePasses();

            // Searches the list of pass class names and returns the index of the first matching result. This index
            // can then be used to lookup a PassCreator. Returns a null handle if no match was found.
            CreatorIndex FindCreatorIndex(Name passClassName);

            // Helper function that creates a pass using an index into the list of PassCreators
            Ptr<Pass> CreatePassFromIndex(CreatorIndex index, Name passName, const AZStd::shared_ptr<const PassTemplate>& passTemplate, const PassRequest* passRequest);

            // --- Members ---

            // Cached pointer to the pass library to simplify code
            PassLibrary* m_passLibrary = nullptr;

            // ClassNames are used to look up PassCreators. This list is 1-to-1 with the PassCreator list
            AZStd::vector<Name> m_passClassNames;

            // List of PassCreators that the PassFactory uses to create Passes
            AZStd::vector<PassCreator> m_creationFunctions;
        };
    }   // namespace RPI
}   // namespace AZ
