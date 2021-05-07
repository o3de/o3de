//---------------------------------------------------------------------------------------
// Loads and processes TressFX files.
// Inputs are binary files/streams/blobs
// Outputs are raw data that will mostly end up on the GPU.
//-------------------------------------------------------------------------------------
//
// Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#pragma once

//#ifndef AMD_TRESSFX_ASSET_H
//#define AMD_TRESSFX_ASSET_H

#include <TressFX/AMD_Types.h>
//#include <TressFX/TressFXCommon.h>
#include <TressFX/AMD_TressFX.h>
#include <Math/Transform.h>

#include <TressFX/TressFXCommon.h>


#include <assert.h>
#include <vector>
#include <string>

#include <AzCore/std/containers/unordered_map.h>

namespace AZ::Data
{
    class AssetDataStream;
}

namespace AMD
{
#define TRESSFX_MAX_INFLUENTIAL_BONE_COUNT 4

    static constexpr char* TFXFileExtension = "tfx";                // Contains the hair vertices data.
    static constexpr char* TFXBoneFileExtension = "tfxbone";        // Contains the hair skinning data.
    static constexpr char* TFXMeshFileExtension = "tfxmesh";        // Contains the hair collision object.
    static constexpr char* TFXCombinedFileExtension = "tfxhair";    // A container file with all the above data.

    // The header struct for the .tfxhair file generated in cache. The .tfxhair file is a combined file of .tfx, .tfxbone
    // and .tfxmesh.
    struct TressFXCombinedHairFileHeader
    {
        uint64 offsetTFX = 0;
        uint64 offsetTFXBone = 0;
        uint64 offsetTFXMesh = 0;
    };

    struct TressFXBoneSkinningData
    {
        float boneIndex[TRESSFX_MAX_INFLUENTIAL_BONE_COUNT];    // [To Do] Adi: check if can be changed to uint32_t (march GPU side)
        float weight[TRESSFX_MAX_INFLUENTIAL_BONE_COUNT];
    };

    //class EI_Scene;

    #define EI_Read(ptr, size, pFile) fread(ptr, size, 1, pFile)
    #define EI_Seek(pFile, offset) fseek(pFile, offset, SEEK_SET)

    class TressFXAsset
    {
    public:
        TressFXAsset();
        ~TressFXAsset();

        // Hair data from *.tfx
        std::vector<Vector3> m_positions;   // Inspite of the confusing name, this is actually Vector4 with w=1.0
        std::vector<AMD::float2> m_strandUV;
        std::vector<AMD::float4> m_tangents;
        std::vector<Vector3> m_followRootOffsets;
        std::vector<AMD::int32>      m_strandTypes;
        std::vector<AMD::real32>     m_thicknessCoeffs;
        std::vector<AMD::real32>     m_restLengths;
        std::vector<AMD::int32>      m_triangleIndices;


        // Bone skinning data from *.tfxbone - this represents the bones Id affecting hairs (not that actual bone structure)
        std::vector<TressFXBoneSkinningData> m_boneSkinningData;

        // Stores a mapping of the bone index to actual bone names.
        std::vector<std::string> m_boneNames;
        using BoneIndexMap = AZStd::vector<uint32>; // Index -> TressFXBoneIndex
                                                    // Value -> EngineBoneIndex (global)

        // When the skinning data populated from the asset, it will be stored as local bone indices. We need to fix the
        // bone indices in the skinning data to engineBoneIndex in the hairComponent when we have access to the complete actor skeleton.
        bool m_boneIndicesFixed = false;

        // counts on hair data
        AMD::int32 m_numTotalStrands;
        AMD::int32 m_numTotalVertices;
        AMD::int32 m_numVerticesPerStrand;
        AMD::int32 m_numGuideStrands;
        AMD::int32 m_numGuideVertices;
        AMD::int32 m_numFollowStrandsPerGuide;

        // Loads *.tfx hair data 
        bool LoadHairData(FILE* ioObject);

        //Generates follow hairs procedually.  If numFollowHairsPerGuideHair is zero, then this function won't do anything. 
        bool GenerateFollowHairs(int numFollowHairsPerGuideHair = 0, float tipSeparationFactor = 0, float maxRadiusAroundGuideHair = 0);

        // Computes various parameters for simulation and rendering. After calling this function, data is ready to be passed to hair object. 
        bool ProcessAsset();

        //! Given the *.tfxbon file that contains bones hair skinning data, this method returns
        //!  the used bones names array of the current hair object so it can be matched with
        //!  the skeleton bones names to get the indices in the global skeleton array.
        void GetBonesNames(FILE* ioObject, std::vector<std::string>& boneNames);
        //! Given the bones hair skinning data file and the pairing local indices to skeleton indices array,
        //!  this method convert all bones indices per vertex and creates the skinning data array. 
        bool LoadBoneData(FILE* ioObject, std::vector<int32_t> skeletonBoneIndices);

        // This function load the combined hair data (.tfxhair file) generated in the cache using the custom builder.
        // Inside the function it will call loadhairdata, loadboneData and loadcollisiondata function. The TressFX asset
        // will be fully populated after this function return true.
        bool LoadCombinedHairData(AZ::Data::AssetDataStream* stream);

        // Similar function to the above but read the assetDataStream instead. Those function will eventually replacing their counterpart.
        // Notice: The loadCombineHairData will control the seek position of the stream, which guranteed those stream will be at the correct
        // position when entering those function.
        bool LoadHairData(AZ::Data::AssetDataStream* stream);
        void GetBonesNames(AZ::Data::AssetDataStream* stream, std::vector<std::string>& boneNames);
        bool LoadBoneData(AZ::Data::AssetDataStream* stream);

        void FixBoneIndices(const BoneIndexMap& boneIndexMap);

        inline AMD::uint32 GetNumHairSegments() { return m_numTotalStrands * (m_numVerticesPerStrand - 1); }
        inline AMD::uint32 GetNumHairTriangleIndices() { return 6 * GetNumHairSegments(); }
        inline AMD::uint32 GetNumHairLineIndices() { return 2 * GetNumHairSegments(); }

    private:

        // Helper functions for ProcessAsset. 
        void ComputeThicknessCoeffs();
        void ComputeStrandTangent();
        void ComputeRestLengths();
        void FillTriangleIndexArray();
    };
    // #endif // AMD_TRESSFX_ASSET_H

} // namespace AMD

