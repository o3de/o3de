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

#include "LmbrCentral_precompiled.h"
#include <LmbrCentral/Rendering/MaterialHandle.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Color.h>
#include <IRenderer.h>
#include <ISystem.h>
#include <I3DEngine.h>


namespace LmbrCentral
{
    // Provides a set of reflected functions for operating on IMaterial through a MaterialHandle
    // We keep these cpp-private instead of putting them in the header to align with the fact that
    // MaterialHandle is not useful on the code side, and only exists to support reflection. If it
    // were possible to reflect IMaterial directly, we'd still have this same set of functions here.
    namespace MaterialHandleFunctions
    {
        void SetParamVector4(MaterialHandle* thisPtr, const AZStd::string& name, const AZ::Vector4& value)
        {
            if (thisPtr && thisPtr->m_material)
            {
                if (!thisPtr->m_material->IsMaterialGroup())
                {
                    Vec4 vec4(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
                    thisPtr->m_material->SetGetMaterialParamVec4(name.c_str(), vec4, false, true);
                }
                else
                {
                    AZ_Error("Material", false, "SetParamVector4 only accepts single Materials, not Material Groups");
                }
            }
            else
            {
                AZ_Warning("Material", false, "Invalid Material passed to SetParamVector4");
            }
        }

        void SetParamVector3(MaterialHandle* thisPtr, const AZStd::string& name, const AZ::Vector3& value)
        {
            if (thisPtr && thisPtr->m_material)
            {
                if (!thisPtr->m_material->IsMaterialGroup())
                {
                    Vec3 vec3(value.GetX(), value.GetY(), value.GetZ());
                    thisPtr->m_material->SetGetMaterialParamVec3(name.c_str(), vec3, false, true);
                }
                else
                {
                    AZ_Error("Material", false, "SetParamVector3 only accepts single Materials, not Material Groups");
                }

            }
            else
            {
                AZ_Warning("Material", false, "Invalid Material passed to SetParamVector3");
            }
        }

        void SetParamColor(MaterialHandle* thisPtr, const AZStd::string& name, const AZ::Color& value)
        {
            if (thisPtr && thisPtr->m_material)
            {
                if (!thisPtr->m_material->IsMaterialGroup())
                {
                    // When value had garbage data is was not only making the material render black, it also corrupted something
                    // on the GPU, making black boxes flicker over the sky.
                    // It was garbage due to a bug in the Color object node where all fields have to be set to some value manually; the default is not 0.
                    if ((value.GetR() < 0 || value.GetR() > 1) ||
                        (value.GetG() < 0 || value.GetG() > 1) ||
                        (value.GetB() < 0 || value.GetB() > 1) ||
                        (value.GetA() < 0 || value.GetA() > 1))
                    {
                        return;
                    }

                    Vec4 vec4(value.GetR(), value.GetG(), value.GetB(), value.GetA());
                    thisPtr->m_material->SetGetMaterialParamVec4(name.c_str(), vec4, false, true);
                }
                else
                {
                    AZ_Error("Material", false, "SetParamColor only accepts single Materials, not Material Groups");
                }
            }
            else
            {
                AZ_Warning("Material", false, "Invalid Material passed to SetParamColor");
            }
        }

        void SetParamFloat(MaterialHandle* thisPtr, const AZStd::string& name, float value)
        {
            if (thisPtr && thisPtr->m_material)
            {
                if (!thisPtr->m_material->IsMaterialGroup())
                {
                    thisPtr->m_material->SetGetMaterialParamFloat(name.c_str(), value, false, true);
                }
                else
                {
                    AZ_Error("Material", false, "SetParamFloat only accepts single Materials, not Material Groups");
                }
            }
            else
            {
                AZ_Warning("Material", false, "Invalid Material passed to SetParamFloat");
            }
        }

        AZ::Vector4 GetParamVector4(MaterialHandle* thisPtr, const AZStd::string& name)
        {
            AZ::Vector4 value = AZ::Vector4::CreateZero();

            if (thisPtr && thisPtr->m_material)
            {
                if (!thisPtr->m_material->IsMaterialGroup())
                {
                    Vec4 vec4;
                    if (thisPtr->m_material->SetGetMaterialParamVec4(name.c_str(), vec4, true, true))
                    {
                        value.Set(vec4.x, vec4.y, vec4.z, vec4.w);
                    }
                }
                else
                {
                    AZ_Error("Material", false, "GetParamVector4 only accepts single Materials, not Material Groups");
                }
            }
            else
            {
                AZ_Warning("Material", false, "Invalid Material passed to GetParamVector4");
            }

            return value;
        }

        AZ::Vector3 GetParamVector3(MaterialHandle* thisPtr, const AZStd::string& name)
        {
            AZ::Vector3 value = AZ::Vector3::CreateZero();

            if (thisPtr && thisPtr->m_material)
            {
                if (!thisPtr->m_material->IsMaterialGroup())
                {
                    Vec3 vec3;
                    if (thisPtr->m_material->SetGetMaterialParamVec3(name.c_str(), vec3, true, true))
                    {
                        value.Set(vec3.x, vec3.y, vec3.z);
                    }
                }
                else
                {
                    AZ_Error("Material", false, "GetParamVector3 only accepts single Materials, not Material Groups");
                }

            }
            else
            {
                AZ_Warning("Material", false, "Invalid Material passed to GetParamVector3");
            }

            return value;
        }

        AZ::Color GetParamColor(MaterialHandle* thisPtr, const AZStd::string& name)
        {
            AZ::Color value = AZ::Color::CreateZero();

            if (thisPtr && thisPtr->m_material)
            {
                if (!thisPtr->m_material->IsMaterialGroup())
                {
                    Vec4 vec4;
                    if (thisPtr->m_material->SetGetMaterialParamVec4(name.c_str(), vec4, true, true))
                    {
                        value.Set(vec4.x, vec4.y, vec4.z, vec4.w);
                    }
                }
                else
                {
                    AZ_Error("Material", false, "GetParamColor only accepts single Materials, not Material Groups");
                }

            }
            else
            {
                AZ_Warning("Material", false, "Invalid Material passed to GetParamColor");
            }

            return value;
        }

        float GetParamFloat(MaterialHandle* thisPtr, const AZStd::string& name)
        {
            float value = 0.0f;

            if (thisPtr && thisPtr->m_material)
            {
                if (!thisPtr->m_material->IsMaterialGroup())
                {
                    thisPtr->m_material->SetGetMaterialParamFloat(name.c_str(), value, true, true);
                }
                else
                {
                    AZ_Error("Material", false, "GetParamFloat only accepts single Materials, not Material Groups");
                }

            }
            else
            {
                AZ_Warning("Material", false, "Invalid Material passed to GetParamFloat");
            }

            return value;
        }

        MaterialHandle Clone(MaterialHandle* thisPtr)
        {
            MaterialHandle copy;

            if (thisPtr && thisPtr->m_material)
            {
                if (!thisPtr->m_material->IsSubMaterial())
                {
                    copy.m_material = gEnv->p3DEngine->GetMaterialManager()->CloneMultiMaterial(thisPtr->m_material);
                }
                else
                {
                    AZ_Error("Material", false, "Clone does not support Sub-Materials");
                }
            }
            else
            {
                AZ_Warning("Material", false, "Invalid Material passed to Clone");
            }

            return copy;
        }

        MaterialHandle FindByName(const AZStd::string& name)
        {
            MaterialHandle found;
            found.m_material = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(name.c_str());
            return found;
        }

        MaterialHandle LoadByName(const AZStd::string& name)
        {
            LmbrCentral::MaterialHandle handle;
            handle.m_material = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(name.c_str(), false, false, IMaterialManager::ELoadingFlagsPreviewMode);

            AZ_Error(nullptr, handle.m_material, "Material.LoadByName('%s') failed", name.c_str());

            return handle;
        }

        _smart_ptr<IMaterial> GetSubMaterialHelper(_smart_ptr<IMaterial> materialGroup, int materialId)
        {
            if (materialGroup)
            {
                if (materialGroup->IsMaterialGroup())
                {
                    int subMtlCount = materialGroup->GetSubMtlCount();
                    if (materialId >= 1 && materialId <= subMtlCount)
                    {
                        return materialGroup->GetSubMtl(materialId-1);
                    }
                    else
                    {
                        AZ_Error("Material", false, "Invalid Material ID %d passed to FindSubMaterial. %d Materials are available.", materialId, subMtlCount);
                    }
                }
                else
                {
                    AZ_Error("Material", false, "FindSubMaterial does not support single Material");
                }
            }
            else
            {
                AZ_Warning("Material", false, "Invalid Material passed to FindSubMaterial.");
            }

            return nullptr;
        }



        MaterialHandle FindSubMaterial(const AZStd::string& name, int id, bool shouldLoad)
        {
            MaterialHandle found;
            _smart_ptr<IMaterial> materialGroup = gEnv->p3DEngine->GetMaterialManager()->FindMaterial(name.c_str());
            if (materialGroup)
            {
                found.m_material = GetSubMaterialHelper(materialGroup, id);
            }
            else
            {
                if (shouldLoad)
                {
                    materialGroup = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(name.c_str(), false, false, IMaterialManager::ELoadingFlagsPreviewMode);
                    if (materialGroup)
                    {
                        found.m_material = GetSubMaterialHelper(materialGroup, id);
                    }
                    else
                    {
                        AZ_Error("Material", false, "Load Material '%s' failed", name.c_str());
                    }
                }
                else
                {
                    AZ_Warning("Material", false, "No Sub-Material is found since Material '%s' is not loaded", name.c_str());
                }
            }

            return found;
        }

        AZStd::string ToString(MaterialHandle* thisPtr)
        {
            if (!thisPtr || !thisPtr->m_material)
            {
                return "Invalid";
            }
            else
            {
                return thisPtr->m_material->GetName();
            }
        }
    }

    void MaterialHandle::Reflect(AZ::SerializeContext* serializeContext)
    {
        // This is required in order to create a MaterialHandle variable in script canvas.
        serializeContext->Class<MaterialHandle>()->Version(0);
    }

    void MaterialHandle::Reflect(AZ::BehaviorContext* behaviorContext)
    {
        const char* setMaterialParamTooltip = "Sets a Material param value";
        const char* getMaterialParamTooltip = "Returns a Material param value";
        AZ::BehaviorParameterOverrides setMaterialDetails = { "Material", "The Material to modify" };
        AZ::BehaviorParameterOverrides getMaterialDetails = { "Material", "The Material to inspect" };
        AZ::BehaviorParameterOverrides setParamNameDetails = { "ParamName", "The name of the Material param to set" };
        AZ::BehaviorParameterOverrides getParamNameDetails = { "ParamName", "The name of the Material param to return" };
        const AZStd::array<AZ::BehaviorParameterOverrides, 2> getMaterialParamArgs = { { getMaterialDetails,getParamNameDetails } };
        const char* newValueTooltip = "The new value to apply";
        
        behaviorContext->Class<MaterialHandle>("Material")
            ->Attribute(AZ::Script::Attributes::Category, "Rendering")
            ->Method("ToString", &MaterialHandleFunctions::ToString)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All) // Hide this node because it doesn't really make sense for the user (GetName would be better), but we need "ToString" in order to provide nice output in Material Variable nodes in script canvas.
            ->Method("FindByName", &MaterialHandleFunctions::FindByName, { { { "Name", "Full path name of the Material" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Find a Material by name. Returns Invalid if the Material is not already loaded.")
            ->Method("LoadByName", &MaterialHandleFunctions::LoadByName, { { { "Name", "Full path name of the Material" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Find a Material by name, loading the asset if needed. Returns Invalid if the Material could not be found or loaded.")
            ->Method("Clone", &MaterialHandleFunctions::Clone, { { { "Material", "The Material to clone" } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Creates a copy of the given Material.")
            ->Method("FindSubMaterial", &MaterialHandleFunctions::FindSubMaterial, 
                { { { "Name", "Full path name of the Material Group to get a Sub-Material from" },
                    { "MaterialID", "The ID of a Sub-Material to access. IDs start at 1.",  behaviorContext->MakeDefaultValue(1) },
                    { "ShouldLoad", "Whether to load the Material Group or not if it's not loaded",  behaviorContext->MakeDefaultValue(true) } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, "Find a Sub-Material from a Material Group by specified Material ID. Returns Invalid if the Material Group could not be found or loaded or the Sub-Material could not be found.")
            ->Method("SetParamVector4", &MaterialHandleFunctions::SetParamVector4, 
                { { setMaterialDetails,setParamNameDetails,{ "Vector4", newValueTooltip } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialParamTooltip)
            ->Method("SetParamVector3", &MaterialHandleFunctions::SetParamVector3, 
                { { setMaterialDetails,setParamNameDetails,{ "Vector3", newValueTooltip } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialParamTooltip)
            ->Method("SetParamColor", &MaterialHandleFunctions::SetParamColor, 
                { { setMaterialDetails,setParamNameDetails,{ "Color", newValueTooltip } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialParamTooltip)
            ->Method("SetParamNumber", &MaterialHandleFunctions::SetParamFloat, // Using "ParamNumber" instead of "ParamFloat" because in Script Canvas all primitives are just "numbers"
                { { setMaterialDetails,setParamNameDetails,{ "Number", newValueTooltip } } })
                ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialParamTooltip)
            ->Method("GetParamVector4", &MaterialHandleFunctions::GetParamVector4, getMaterialParamArgs)
                ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialParamTooltip)
            ->Method("GetParamVector3", &MaterialHandleFunctions::GetParamVector3, getMaterialParamArgs)
                ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialParamTooltip)
            ->Method("GetParamColor", &MaterialHandleFunctions::GetParamColor, getMaterialParamArgs)
                ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialParamTooltip)
            ->Method("GetParamNumber", &MaterialHandleFunctions::GetParamFloat, getMaterialParamArgs) // Using "ParamNumber" instead of "ParamFloat" because in Script Canvas all primitives are just "numbers"
                ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialParamTooltip)
            ;
    }
}
