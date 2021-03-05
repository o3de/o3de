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

#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>

namespace AZ
{
    namespace Render
    {
        //! Materials can use this functor to control when and how to set the visibility of a group of properties.
        class PropertyVisibilityFunctor final
            : public RPI::MaterialFunctor
        {
            friend class PropertyVisibilityFunctorSourceData;
        public:
            AZ_RTTI(AZ::Render::PropertyVisibilityFunctor, "{2582B36F-FA7C-450F-B46A-39AAE18356A0}", RPI::MaterialFunctor);

            static void Reflect(ReflectContext* context);

            void Process(EditorContext& context) override;

        private:
            struct Action
            {
                AZ_TYPE_INFO(AZ::Render::PropertyVisibilityFunctor::Action, "{5DF4D981-9D0C-4040-A6C5-52E1D0BD876B}");

                RPI::MaterialPropertyIndex      m_triggerPropertyIndex; //! The control property for affected properties.
                RPI::MaterialPropertyValue      m_triggerValue;         //! The trigger value of the control property.
                RPI::MaterialPropertyVisibility m_visibility;           //! The visibility of affected properties when the trigger value is hit.
            };
            // Material property inputs...
            AZStd::vector<Action>                     m_actions;            //! The actions that describes when and what to do with visibilities.
            AZStd::vector<RPI::MaterialPropertyIndex> m_affectedProperties; //! The properties that are affected by actions.
        };

    } // namespace Render
} // namespace AZ
