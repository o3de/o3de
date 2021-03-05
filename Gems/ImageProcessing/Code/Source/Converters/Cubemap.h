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

#include <Processing/ImageToProcess.h>

namespace ImageProcessing
{
    // note: lumberyard is right hand Z up coordinate
    // please don't change the order of the enum since we are using it to match the face id defined in AMD's CubemapGen
    // and they are using left hand Y up coordinate
    enum CubemapFace
    {
        FaceLeft = 0,
        FaceRight,
        FaceFront,
        FaceBack,
        FaceTop,
        FaceBottom,
        FaceCount
    };

    //we are treating the orientation of faces in 4x3 layout as the original direction.
    enum class CubemapFaceDirection
    {
        DirNoRotation = 0,
        DirRotateLeft90,
        DirRotateRight90,
        DirRotate180,
        DirMirrorHorizontal
    };

    //this class contains information to describe a cubemap layout
    class CubemapLayoutInfo
    {
    public:
        struct FaceInfo
        {
            AZ::u8 row;
            AZ::u8 column;
            CubemapFaceDirection direction;
        };

        //rows and columns of how cubemap's faces laid
        AZ::u8 m_rows;
        AZ::u8 m_columns;

        //the type of this layout info for
        CubemapLayoutType m_type;

        //the index of row and column where all the faces located
        FaceInfo m_faceInfos[FaceCount];

        CubemapLayoutInfo();
        void SetFaceInfo(CubemapFace face, AZ::u8 row, AZ::u8 col, CubemapFaceDirection dir);
    };

    //class to help doing operations with faces for an image as cubemap
    class CubemapLayout
    {
    public:
        //create a cubemapLayout object for the image. It can be used later to get image information as a cubemap
        static CubemapLayout* CreateCubemapLayout(IImageObjectPtr image);

        //get layout info for input layout type
        static CubemapLayoutInfo* GetCubemapLayoutInfo(CubemapLayoutType type);

        //get layout info for input image based on its size
        static CubemapLayoutInfo* GetCubemapLayoutInfo(IImageObjectPtr image);

        //public functions to get faces information for associated image
        AZ::u32 GetFaceSize();

        //get the rect where the face in the image 
        void GetRectForFace(AZ::u32 mip, CubemapFace face, QRect& outRect);

        CubemapLayoutInfo* GetLayoutInfo();

        //set/get pixels' data from/to specific face. only works for mip 0
        void GetFaceData(CubemapFace face, void* outBuffer, AZ::u32& outSize);
        void SetFaceData(CubemapFace face, void* dataBuffer, AZ::u32 dataSize);

        //get the face's direction
        CubemapFaceDirection GetFaceDirection(CubemapFace face);

        //get memory for a face from Image data. only works for CubemapLayoutVertical since its memory for each face is continuous
        void* GetFaceMemBuffer(AZ::u32 mip, CubemapFace face, AZ::u32& outPitch);
        void SetToFaceMemBuffer(AZ::u32 mip, CubemapFace face, void* dataBuffer);

    private:
        //information for all supported cubemap layouts
        static CubemapLayoutInfo s_layoutList[CubemapLayoutTypeCount];

        //the image associated for this CubemapLayout
        IImageObjectPtr m_image;
        //the layout information of m_image
        CubemapLayoutInfo *m_info;
        //the size of the cubemap's face (which is square and power of 2). 
        uint32 m_faceSize;

        //private constructor. User should always use CreateCubemapLayout create a layout for an image object
        CubemapLayout();

        //initialize information of all available cubemap layouts
        static void InitCubemapLayoutInfos();
    };

}//end namspace ImageProcessing