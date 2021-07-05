/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

#include <AzCore/Asset/AssetCommon.h>

namespace UnitTest
{
    class PassBuilderTests;
}

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! An asset that describes the root of a pass by having a PassTemplate.
        //! By adding PassRequests to the template you can describe an entire tree of passes.
        class PassAsset final
            : public Data::AssetData
        {
            friend class PassSystem;
            friend class UnitTest::PassBuilderTests;
        public:
            AZ_RTTI(PassAsset, "{FBAF94C2-6617-491E-8269-55DBC9845539}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(PassAsset, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            static const char* DisplayName;
            static const char* Group;
            static const char* Extension;

            //! Retrieves the underlying PassTemplate
            const AZStd::unique_ptr<PassTemplate>& GetPassTemplate() const;

        private:
            // Sets the pass template on this pass asset. Use for testing only.
            void SetPassTemplateForTestingOnly(const PassTemplate& passTemplate);

            AZStd::unique_ptr<PassTemplate> m_passTemplate = nullptr;
        };

        using PassAssetHandler = AssetHandler<PassAsset>;

    } // namespace RPI
} // namespace AZ
