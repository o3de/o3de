/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Trace.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <stdarg.h>

namespace AZ
{
    namespace RPI
    {
        //! Common base class for asset creators, which are used to create and initialize instances of an immutable asset class.
        //! 
        //! (Note this generally follows the builder design pattern, but is called a "creator" rather 
        //!  than a "builder" to avoid confusion with the AssetBuilderSDK).
        template<typename AssetDataT>
        class AssetCreator
        {
        public:
            //! When true, treat all subsequent warnings as errors. Any warnings already reported will not be elevated.
            // [GFX TODO] We need to iterate on this concept at some point. We may want to expose it through cvars or something
            // like that, or we may not need this at all. For now it's helpful for testing.
            void SetElevateWarnings(bool elevated);
            bool GetElevateWarnings() const { return m_warningsElevated; }

            int GetErrorCount() const { return m_errorCount; }
            int GetWarningCount() const { return m_warningCount; }
            bool IsFailed() const { return m_errorCount || m_warningsElevated && m_warningCount; }

            //! Errors should be reported for any condition that prevents creating a valid asset that can be used at runtime.
            //! The output asset data would be corrupt to the point that the runtime would report further errors or even crash.
            //! Once an error has been reported, subsequent calls to ValidateIsReady() will return false.
            //! (Normally this will be called by subclasses, but it is public so client code may also report errors in the same way;
            //!  for example, when client code is unable to prepare inputs for the AssetCreator).
            template<typename ... Args>
            void ReportError(const char* format, Args... args);

            //! Warnings should be reported for any condition that indicates a rendered asset may not appear as the user expects.
            //! However, the runtime will not crash or report errors if the output asset is used.
            //! (Normally this will be called by subclasses, but it is public so client code may also report warnings in the same way;
            //!  for example, when client code is unable to prepare inputs for the AssetCreator).
            template<typename ... Args>
            void ReportWarning(const char* format, Args... args);

        protected:
            AssetCreator();
            virtual ~AssetCreator() = default;
            AZ_DISABLE_COPY_MOVE(AssetCreator);

            //! Utility function that creates the m_asset instance that this asset creator will build.
            //! Subclasses should call this at the beginning of their Begin() function.
            void BeginCommon(const Data::AssetId& assetId);

            //! Utility function that finalizes and transfers ownership of m_asset to result, if successful. 
            //! Otherwise returns false and result is left untouched.
            //! Subclasses should call this at the end of their End() function, after making
            //! any final changes to m_asset.
            bool EndCommon(Data::Asset<AssetDataT>& result);

            //! Reports common errors, and returns false if processing should not continue due to prior errors.
            //!
            //! Subclasses should call this function before attempting any manipulation of the m_asset, and return
            //! immediately if it returns false. This alleviates subclasses from having to track custom state; they
            //! just need to call ReportError() for any breaking issue.
            //!
            //! @return false if any errors have been previously reported. Note, it does not return false due to previously 
            //!         reported warnings, because warnings should not invalidate subsequent manipulations of the m_asset.
            bool ValidateIsReady();

            //! Reports errors when a pointer is null
            bool ValidateNotNull(void* pointer, const char* name);
            bool ValidateNotNull(const AZ::Data::Asset<AZ::Data::AssetData>& pointer, const char* name);
            template<typename T>
            bool ValidateNotNull(const AZStd::intrusive_ptr<T>& pointer, const char* name);

            //! This is the asset that subclass creators will build
            Data::Asset<AssetDataT> m_asset;

        private:

            //! Reset error and warning counters. This should be done when reusing a creator for multiple assets.
            //! Subclasses will not need to call this function directly because it's done in AssetCreator::BeginCommon()
            void ResetIssueCounts();

            //! Internal utility for EndCommon() to check the state of the asset creator
            bool ValidateIsAssetBuilt();

            const char* m_assetClassName = nullptr;
            int m_errorCount = 0;
            int m_warningCount = 0;
            bool m_warningsElevated = false;
            bool m_beginCalled = false;
            bool m_abortMessageReported = false;
        };

        template<typename AssetDataT>
        AssetCreator<AssetDataT>::AssetCreator()
        {
            m_assetClassName = AssetDataT::RTTI_TypeName();
        }

        template<typename AssetDataT>
        void AssetCreator<AssetDataT>::SetElevateWarnings(bool elevated)
        {
            m_warningsElevated = elevated;
        }

        template<typename AssetDataT>
        void AssetCreator<AssetDataT>::BeginCommon(const Data::AssetId& assetId)
        {
            AZ_Assert(!m_beginCalled, "Begin() was already called");
            AZ_Assert(!m_asset, "Asset should be null at Begin()");

            ResetIssueCounts(); // Because the asset creator can be used multiple times

            m_asset = Data::Asset<AssetDataT>(assetId, aznew AssetDataT, AZ::Data::AssetLoadBehavior::PreLoad);
            m_beginCalled = true;

            if (!m_asset)
            {
                ReportError("Failed to create %s", m_assetClassName);
            }
        }

        template<typename AssetDataT>
        bool AssetCreator<AssetDataT>::EndCommon(Data::Asset<AssetDataT>& result)
        {
            bool success = false;

            if (!ValidateIsAssetBuilt())
            {
                m_asset.Release();
            }
            else
            {
                Data::AssetManager::Instance().AssignAssetData(m_asset);
                result = AZStd::move(m_asset);
                success = true;
            }

            // Even if EndCommon() failed, the process is no longer in the Begin state.
            m_beginCalled = false;

            return success;
        }

        template<typename AssetDataT>
        template<typename ... Args>
        void AssetCreator<AssetDataT>::ReportError([[maybe_unused]] const char* format, [[maybe_unused]] Args... args)
        {
            ++m_errorCount;
#if defined(AZ_ENABLE_TRACING) // disabling since it requires argument expansion in this context
            AZ_Error(m_assetClassName, false, format, args...);
#endif
        }

        template<typename AssetDataT>
        template<typename ... Args>
        void AssetCreator<AssetDataT>::ReportWarning([[maybe_unused]] const char* format, [[maybe_unused]] Args... args)
        {
            ++m_warningCount;
#if defined(AZ_ENABLE_TRACING) // disabling since it requires argument expansion in this context
            AZ_Warning(m_assetClassName, false, format, args...);
#endif
        }

        template<typename AssetDataT>
        bool AssetCreator<AssetDataT>::ValidateIsReady()
        {
            if (!m_beginCalled)
            {
                AZ_Assert(false, "Begin() was not called");
                return false;
            }

            if (m_errorCount > 0)
            {
                // We only report this error once because ValidateIsReady() may be called many times before End()
                if (!m_abortMessageReported)
                {
                    ReportError("Cannot continue building %s because %d error(s) reported", m_assetClassName, m_errorCount);
                    m_abortMessageReported = true;
                }

                return false;
            }

            return true;
        }

        template<typename AssetDataT>
        bool AssetCreator<AssetDataT>::ValidateNotNull(void* pointer, const char* name)
        {
            if (!pointer)
            {
                ReportError("%s is null", name);
                return false;
            }
            else
            {
                return true;
            }
        }

        template<typename AssetDataT>
        bool AssetCreator<AssetDataT>::ValidateNotNull(const AZ::Data::Asset<AZ::Data::AssetData>& pointer, const char* name)
        {
            return ValidateNotNull(pointer.Get(), name);
        }

        template<typename AssetDataT>
        template<typename T>
        bool AssetCreator<AssetDataT>::ValidateNotNull(const AZStd::intrusive_ptr<T>& pointer, const char* name)
        {
            return ValidateNotNull(pointer.get(), name);
        }

        template<typename AssetDataT>
        bool AssetCreator<AssetDataT>::ValidateIsAssetBuilt()
        {
            if (!m_beginCalled)
            {
                AZ_Assert(false, "Begin() was not called");
                return false;
            }

            if (m_errorCount > 0)
            {
                if (!m_abortMessageReported)
                {
                    ReportError("Failed to build %s because %d error(s) reported", m_assetClassName, m_errorCount);
                    m_abortMessageReported = true;
                }
                return false;
            }

            if (m_warningsElevated && m_warningCount > 0)
            {
                ReportError("Failed to build %s because %d warning(s) reported", m_assetClassName, m_warningCount);
                return false;
            }

            // We expect subclasses to ensure the asset is in the ready state before EndCommon(), rather than call 
            // m_asset->SetReady() in EndCommon(). Our pattern is for SetReady() to be a private function, and the
            // asset creator leaf class is a friend of the asset class. If we were to make EndCommon() call SetReady()
            // then we would have to make AssetCommon a friend of the asset too, which is a bit ugly.
            if (m_asset->GetStatus() != Data::AssetData::AssetStatus::Ready)
            {
                AZ_Assert(false, "Asset must be put into the Ready state before calling EndCommon().");
                return false;
            }

            return true;
        }

        template<typename AssetDataT>
        void AssetCreator<AssetDataT>::ResetIssueCounts()
        {
            m_errorCount = 0;
            m_warningCount = 0;
            m_abortMessageReported = false;
        }

    } //namespace RPI
} //namespace AZ

