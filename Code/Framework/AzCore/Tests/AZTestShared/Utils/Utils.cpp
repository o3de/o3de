
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AZTestShared/Utils/Utils.h>
#include "AzCore/Component/Entity.h"
#include "AzCore/Asset/AssetManager.h"
#include "AzCore/Slice/SliceComponent.h"

namespace UnitTest
{
    AZStd::string GetTestFolderPath()
    {
        return AZ_TRAIT_TEST_ROOT_FOLDER;
    }

    void MakePathFromTestFolder(char* buffer, int bufferLen, const char* fileName)
    {
        azsnprintf(buffer, bufferLen, "%s%s", GetTestFolderPath().c_str(), fileName);
    }

    ErrorHandler::ErrorHandler(const char* errorPattern)
        : m_errorCount(0)
        , m_warningCount(0)
        , m_expectedErrorCount(0)
        , m_expectedWarningCount(0)
        , m_errorPattern(errorPattern)
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    ErrorHandler::~ErrorHandler()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    int ErrorHandler::GetErrorCount() const
    {
        return m_errorCount;
    }

    int ErrorHandler::GetWarningCount() const
    {
        return m_warningCount;
    }

    int ErrorHandler::GetExpectedErrorCount() const
    {
        return m_expectedErrorCount;
    }

    int ErrorHandler::GetExpectedWarningCount() const
    {
        return m_expectedWarningCount;
    }

    bool ErrorHandler::SuppressExpectedErrors([[maybe_unused]] const char* window, const char* message)
    {
        return AZStd::string(message).find(m_errorPattern) != AZStd::string::npos;
    }

    bool ErrorHandler::OnPreError(
        const char* window, [[maybe_unused]] const char* fileName, [[maybe_unused]] int line,
        [[maybe_unused]] const char* func, const char* message)
    {
        m_errorCount++;
        bool suppress = SuppressExpectedErrors(window, message);
        m_expectedErrorCount += suppress;
        return suppress;
    }

    bool ErrorHandler::OnPreWarning(
        const char* window, [[maybe_unused]] const char* fileName, [[maybe_unused]] int line,
        [[maybe_unused]] const char* func, const char* message)
    {
        m_warningCount++;
        bool suppress = SuppressExpectedErrors(window, message);
        m_expectedWarningCount += suppress;
        return suppress;
    }

    bool ErrorHandler::OnPrintf(const char* window, const char* message)
    {
        return SuppressExpectedErrors(window, message);
    }
}

namespace AZ
{
    namespace Test
    {
        AZ::Data::Asset<AZ::SliceAsset> CreateSliceFromComponent(AZ::Component* assetComponent, AZ::Data::AssetId sliceAssetId)
        {
            auto* sliceEntity = aznew AZ::Entity;
            auto* contentEntity = aznew AZ::Entity;

            if (assetComponent)
            {
                contentEntity->AddComponent(assetComponent);
            }

            auto sliceAsset = Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(sliceAssetId, AZ::Data::AssetLoadBehavior::Default);

            AZ::SliceComponent* component = sliceEntity->CreateComponent<AZ::SliceComponent>();
            component->SetIsDynamic(true);
            sliceAsset.Get()->SetData(sliceEntity, component);
            component->AddEntity(contentEntity);

            return sliceAsset;
        }

        AssertAbsorber::AssertAbsorber()
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        AssertAbsorber::~AssertAbsorber()
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        }

        bool AssertAbsorber::OnPreAssert(const char* fileName, int line, const char* func, const char* message)
        {
            AZ_UNUSED(fileName);
            AZ_UNUSED(line);
            AZ_UNUSED(func);
            AZ_UNUSED(message);
            ++m_assertCount;
            return true;
        }

        bool AssertAbsorber::OnAssert(const char* message)
        {
            AZ_UNUSED(message);
            return true;
        }

        bool AssertAbsorber::OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message)
        {
            AZ_UNUSED(window);
            AZ_UNUSED(fileName);
            AZ_UNUSED(line);
            AZ_UNUSED(func);
            AZ_UNUSED(message);
            ++m_errorCount;
            return true;
        }

        bool AssertAbsorber::OnError(const char* window, const char* message)
        {
            AZ_UNUSED(window);
            AZ_UNUSED(message);
            return true;
        }

        bool AssertAbsorber::OnWarning(const char* window, const char* message)
        {
            AZ_UNUSED(window);
            AZ_UNUSED(message);
            return true;
        }

        bool AssertAbsorber::OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message)
        {
            AZ_UNUSED(window);
            AZ_UNUSED(fileName);
            AZ_UNUSED(line);
            AZ_UNUSED(func);
            AZ_UNUSED(message);
            ++m_warningCount;
            return true;
        }

    }// namespace Test
}// namespace AZ
