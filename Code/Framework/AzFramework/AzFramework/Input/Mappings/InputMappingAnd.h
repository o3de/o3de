/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Mappings/InputMapping.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that maps multiple different sources to a single output using 'AND' logic.
    //! Example: "gamepad_button_L1" AND "gamepad_button_R1" -> "gameplay_strong_attack"
    class InputMappingAnd : public InputMapping
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The input mapping configuration values that are exposed to the editor.
        class Config : public InputMapping::ConfigBase
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Allocator
            AZ_CLASS_ALLOCATOR(Config, AZ::SystemAllocator, 0);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Type Info
            AZ_RTTI(Config, "{54E972F3-0477-4E2E-93F5-4E06ED755DF6}", InputMapping::ConfigBase);

            ////////////////////////////////////////////////////////////////////////////////////////
            // Reflection
            static void Reflect(AZ::ReflectContext* context);

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Destructor
            ~Config() override = default;

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! \ref AzFramework::InputMapping::Type::CreateInputMapping
            AZStd::shared_ptr<InputMapping> CreateInputMapping(const InputContext& inputContext) const override;

        private:
            ////////////////////////////////////////////////////////////////////////////////////////
            //! The source input channel names that will be mapped to the output input channel name.
            AZStd::vector<InputChannelNameFilteredByDeviceType> m_sourceInputChannelNames;
        };

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputMappingAnd, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(InputMappingAnd, "{9ADDAB97-E786-4BB9-99EE-924EE956E56A}", InputMapping);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputChannelId Id of the input mapping
        //! \param[in] inputContext Input context that owns the input mapping
        InputMappingAnd(const InputChannelId& inputChannelId,
                        const InputContext& inputContext);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Disable copying
        AZ_DISABLE_COPY_MOVE(InputMappingAnd);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default Destructor
        ~InputMappingAnd() override = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Add an input source to this input mapping
        //! \param[in] inputSourceId The id of the input source to add to this input mapping
        //! \return True if the input source was added to this input mapping, false otherwise
        bool AddSourceInput(const InputChannelId& inputSourceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Remove an input source from this input mapping
        //! \param[in] inputSourceId The id of the input source to remove from this input mapping
        //! \return True if the input source was removed from this input mapping, false otherwise
        bool RemoveSourceInput(const InputChannelId& inputSourceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the average value of the source input channels currently activating this input
        //! \return The value of the source input channels currently activating this input mapping
        float GetValue() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the average delta of the source input channels currently activating this input
        //! \return Difference between the current and last reported values of this input mapping
        float GetDelta() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelRequests::ResetState
        void ResetState() override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputMapping::IsSourceInput
        bool IsSourceInput(const InputChannel& inputChannel) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputMapping::OnSourceInputEvent
        bool OnSourceInputEvent(const InputChannel& inputChannel) override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Calculate the average value of the active source input channels
        float CalculateAverageValue() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        SourceInputChannelIds m_sourceInputChannelIds; //!< All source input channel ids
        ActiveInputChannelIds m_activeInputChannelIds; //!< Active source input channel/device ids

        float m_averageValue = 0.0f; //!< The average value of the active source input channels
        float m_averageDelta = 0.0f; //!< The delta between the current and last average value
    };
} // namespace AzFramework
