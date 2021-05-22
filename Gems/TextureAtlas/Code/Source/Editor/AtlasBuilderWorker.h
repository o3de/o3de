/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <qimage.h>
#include <TextureAtlas/TextureAtlasBus.h>

namespace TextureAtlasBuilder
{
    //! Struct that is used to communicate input commands
    struct AtlasBuilderInput
    {
        AZ_CLASS_ALLOCATOR(AtlasBuilderInput, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(AtlasBuilderInput, "{F54477F9-1BDE-4274-8CC0-8320A3EF4A42}");

        bool m_forceSquare;
        bool m_forcePowerOf2;
        // Includes a white default texture for the UI to use under certain circumstances
        bool m_includeWhiteTexture;
        int m_maxDimension;
        // At least this much padding will surround each texture except on the edges of the atlas
        int m_padding;
        // Color used in wasted space
        AZ::Color m_unusedColor;
        // A preset to use for the texture atlas image processing
        AZStd::string m_presetName;

        AZStd::vector<AZStd::string> m_filePaths;
        AtlasBuilderInput(): 
            m_forceSquare(false), 
            m_forcePowerOf2(false), 
            m_includeWhiteTexture(true), 
            m_maxDimension(4096),
            m_padding(1),
            // Default color should be a non-transparent color that isn't used often in uis
            m_unusedColor(.235f, .702f, .443f, 1)
        {
        }

        static void Reflect(AZ::ReflectContext* context);

        //! Attempts to read the input from a .texatlas file. "valid" is for reporting exceptions and telling the asset
        //! proccesor to fail the job. Supports parsing through a human readable custom parser.
        static AtlasBuilderInput ReadFromFile(const AZStd::string& path, const AZStd::string& directory, bool& valid);

        //! Resolves any wild cards in paths
        static void AddFilesUsingWildCard(AZStd::vector<AZStd::string>& paths, const AZStd::string& insert);

        //! Removes anything that matches the wildcard
        static void RemoveFilesUsingWildCard(AZStd::vector<AZStd::string>& paths, const AZStd::string& remove);

        //! Compare considering wildcards
        static bool DoesPathnameMatchWildCard(const AZStd::string& rule, const AZStd::string& path);

        //! As FollowsRule but allows extra items after the last '/'
        static bool DoesWildCardDirectoryIncludePathname(const AZStd::string& rule, const AZStd::string& path);

        //! Helper function for DoesPathnameMatchWildCard
        static bool TokenMatchesWildcard(const AZStd::string& rule, const AZStd::string& token);

        //! Resolves any folder paths into image file paths
        static void AddFolderContents(AZStd::vector<AZStd::string>& paths, const AZStd::string& insert, bool& valid);

        //! Resolves remove commands for folders
        static void RemoveFolderContents(AZStd::vector<AZStd::string>& paths, const AZStd::string& remove);
    };

    //! Struct that is used to represent an object with a width and height in pixels
    struct ImageDimension
    {
        int m_width;
        int m_height;

        ImageDimension(int width, int height)
        {
            m_width = width;
            m_height = height;
        }
    };

    //! Typedef for an ImageDimension paired with an integer
    using IndexImageDimension = AZStd::pair<int, ImageDimension>;

    //! Typedef for a list of ImageDimensions paired with integers
    using ImageDimensionData = AZStd::vector<IndexImageDimension>;

    //! Typedef to simplify references to TextureAtlas::AtlasCoordinates
    using AtlasCoordinates = TextureAtlasNamespace::AtlasCoordinates;

    //! Number of bytes in a pixel
    const int bytesPerPixel = 4;

    //! The size of the padded sorting units (important for compression)
    const int cellSize = 4;

    //! Indexes of the products
    enum class Product
    {
        TexatlasidxProduct = 0,
        DdsProduct = 1
    };

    //! An asset builder for texture atlases
    class AtlasBuilderWorker : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(AtlasBuilderWorker, "{79036188-E017-4575-9EC0-8D39CB560EA6}");

        AtlasBuilderWorker() = default;
        ~AtlasBuilderWorker() = default;

        //! Asset Builder Callback Functions

        //! Called by asset processor to gather information on a job for a ".texatlas" file
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request,
            AssetBuilderSDK::CreateJobsResponse& response);
        //! Called by asset proccessor when it wants us to execute a job
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request,
            AssetBuilderSDK::ProcessJobResponse& response);

        //! Returns the job related information used by the builder
        static AssetBuilderSDK::JobDescriptor GetJobDescriptor(const AZStd::string& sourceFile, const AtlasBuilderInput& input);

        //////////////////////////////////////////////////////////////////////////
        //! AssetBuilderSDK::AssetBuilderCommandBus interface
        void ShutDown() override; // if you get this you must fail all existing jobs and return.
        //////////////////////////////////////////////////////////////////////////

    private:
        bool m_isShuttingDown = false;

        //! This is the main function that takes a set of inputs and attempts to pack them into an atlas of a given
        //! size. Returns true if succesful, does not update out on failure.
        static bool TryPack(const ImageDimensionData& images,
            int targetWidth,
            int targetHeight,
            int padding,
            size_t& amountFit,
            AZStd::vector<AtlasCoordinates>& out);

        //! Removes any overlap between slotList and the given item
        static void TrimOverlap(AZStd::vector<AtlasCoordinates>& slotList, AtlasCoordinates item);

        //! Uses the proper tightening method based on the input and returns the maximum number of items that were able to be fit
        bool TryTightening(AtlasBuilderInput input,
            const ImageDimensionData& images,
            int smallestWidth,
            int smallestHeight,
            int targetArea,
            int padding,
            int& resultWidth,
            int& resultHeight,
            size_t& amountFit,
            AZStd::vector<AtlasCoordinates>& out);

        //! Finds the tightest square fit achievable by expanding a square area until a valid fit is found
        bool TryTighteningSquare(const ImageDimensionData& images,
            int lowerBound,
            int maxDimension,
            int targetArea,
            bool powerOfTwo,
            int padding,
            int& resultWidth,
            int& resultHeight,
            size_t& amountFit,
            AZStd::vector<AtlasCoordinates>& out);

        //! Finds the tightest fit achievable by starting with the optimal thin solution and attempting to resize to be
        //! a better shape
        bool TryTighteningOptimal(const ImageDimensionData& images,
            int smallestWidth,
            int smallestHeight,
            int maxDimension,
            int targetArea,
            bool powerOfTwo,
            int padding,
            int& resultWidth,
            int& resultHeight,
            size_t& amountFit,
            AZStd::vector<AtlasCoordinates>& out);

        //! Sorting logic for adding a slot to a sorted list in order to maintain increasing order
        static void InsertInOrder(AZStd::vector<AtlasCoordinates>& slotList, AtlasCoordinates item);

        //! Misc Logic For Estimating Target Shape

        //! Returns the width of the widest element
        static int GetWidest(const ImageDimensionData& imageList);

        //! Returns the height of the tallest area
        static int GetTallest(const ImageDimensionData& imageList);
    };

    //! Used for sorting ImageDimensions
    static bool operator<(ImageDimension a, ImageDimension b);

    //! Used to expose the ImageDimension in a pair to AZStd::Sort
    static bool operator<(IndexImageDimension a, IndexImageDimension b);

    //! Returns true if two coordinate sets overlap
    static bool Collides(AtlasCoordinates a, AtlasCoordinates b);

    //! Returns true if item collides with any object in list
    static bool Collides(AtlasCoordinates item, AZStd::vector<AtlasCoordinates> list);

    //! Returns the portion of the second item that overlaps with the first
    static AtlasCoordinates GetOverlap(AtlasCoordinates a, AtlasCoordinates b);

    //! Performs an operation that copies a pixel to the output
    static void SetPixels(AZ::u8* dest, const AZ::u8* source, int destBytes);

    //! Checks if we can insert an image into a slot
    static bool CanInsert(AtlasCoordinates slot, ImageDimension image, int padding, int farRight, int farBot);

    //! Adds the necessary padding to an Atlas Coordinate 
    static void AddPadding(AtlasCoordinates& slot, int padding, int farRight, int farBot);
}
