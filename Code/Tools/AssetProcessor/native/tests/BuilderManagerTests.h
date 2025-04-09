/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <native/utilities/BuilderManager.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace UnitTests
{
    class TestBuilder : public AssetProcessor::Builder
    {
    public:
        TestBuilder(const AssetUtilities::QuitListener& quitListener, AZ::Uuid uuid, int connectionId)
            : Builder(quitListener, uuid)
        {
            m_connectionId = connectionId;
        }

    protected:
        AZ::Outcome<void, AZStd::string> Start(AssetProcessor::BuilderPurpose purpose) override;
    };

    class TestBuilderManager : public AssetProcessor::BuilderManager
    {
    public:
        TestBuilderManager(ConnectionManager* connectionManager);

        int GetBuilderCreationCount() const;

    protected:
        AZStd::shared_ptr<AssetProcessor::Builder> AddNewBuilder(AssetProcessor::BuilderPurpose purpose) override;

        int m_connectionCounter = 0;
    };
}
