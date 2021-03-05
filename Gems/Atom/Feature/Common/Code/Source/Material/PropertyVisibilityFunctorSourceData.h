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

#include "PropertyVisibilityFunctor.h"
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialPropertyValueSourceData.h>

namespace AZ
{
    namespace Render
    {
        //! Builds a PropertyVisibilityFunctor.
        //! Materials can use this functor to control whether a specific property group will be enabled.
        class PropertyVisibilityFunctorSourceData final
            : public RPI::MaterialFunctorSourceData
        {
        public:
            AZ_RTTI(AZ::Render::PropertyVisibilityFunctorSourceData, "{B44E6929-8FFF-405F-9056-B9B811F97676}", RPI::MaterialFunctorSourceData);

            static void Reflect(ReflectContext* context);

            FunctorResult CreateFunctor(const EditorContext& context) const override;
        private:
            struct ActionSourceData
            {
                AZ_TYPE_INFO(AZ::Render::PropertyVisibilityFunctorSourceData::ActionSourceData, "{70E01DA6-0B42-4CCB-AAD0-51980DB43F62}");
                AZStd::string                        m_triggerPropertyName; //! The control property for affected properties.
                RPI::MaterialPropertyValueSourceData m_triggerValue;        //! The trigger value of the control property.
                RPI::MaterialPropertyVisibility      m_visibility;          //! The visibility of affected properties when the trigger value is hit.
            };
            // Material property inputs...
            AZStd::vector<ActionSourceData> m_actions;               //! The actions that describes when and what to do with visibilities.
            AZStd::vector<AZStd::string>    m_affectedPropertyNames; //! The properties that are affected by actions.
        };

    } // namespace Render
} // namespace AZ
