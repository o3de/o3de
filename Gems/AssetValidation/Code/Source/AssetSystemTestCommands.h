/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Console/IConsole.h>

namespace AssetValidation
{
    class AssetValidation
    {
        public:
            AssetValidation() = default;
            ~AssetValidation() = default;

        private:
            // Register TestChangeAssets as a console command.
            void TestChangeAssets(const AZ::ConsoleCommandContainer& someStrings);
            AZ_CONSOLEFUNC(AssetValidation, TestChangeAssets, AZ::ConsoleFunctorFlags::Null,
                "Perform series of randomized asset change updates to stress asset reload systems");
    };
}

