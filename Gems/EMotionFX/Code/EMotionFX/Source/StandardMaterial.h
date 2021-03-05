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

// include the Core system
#include "EMotionFXConfig.h"
#include "Material.h"
#include "BaseObject.h"
#include <MCore/Source/Color.h>
#include <MCore/Source/StringIdPool.h>


namespace EMotionFX
{
    /**
     * The material layer class.
     * A material layer is a texture layer in a material.
     * Examples of layers can be, bumpmaps, diffuse maps, opacity maps, specular maps, etc.
     */
    class EMFX_API StandardMaterialLayer
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Standard supported layer types.
         */
        enum
        {
            LAYERTYPE_UNKNOWN           = 0,    /**< An unknown layer type. */
            LAYERTYPE_AMBIENT           = 1,    /**< Ambient layer. */
            LAYERTYPE_DIFFUSE           = 2,    /**< Diffuse layer. */
            LAYERTYPE_SPECULAR          = 3,    /**< Specular layer. */
            LAYERTYPE_OPACITY           = 4,    /**< Opacity layer. */
            LAYERTYPE_BUMP              = 5,    /**< Bumpmap layer. */
            LAYERTYPE_SELFILLUM         = 6,    /**< Self illumination layer. */
            LAYERTYPE_SHINE             = 7,    /**< Shininess layer. */
            LAYERTYPE_SHINESTRENGTH     = 8,    /**< Shininess strength layer. */
            LAYERTYPE_FILTERCOLOR       = 9,    /**< Filter color layer. */
            LAYERTYPE_REFLECT           = 10,   /**< Reflection layer. */
            LAYERTYPE_REFRACT           = 11,   /**< Refraction layer. */
            LAYERTYPE_ENVIRONMENT       = 12,   /**< Environment map layer. */
            LAYERTYPE_DISPLACEMENTCOLOR = 13,   /**< Displacement map layer. */
            LAYERTYPE_DISPLACEMENTFACTOR = 14,   /**< Displacment factor layer. */
            LAYERTYPE_NORMALMAP         = 15    /**< Normal map layer. */
        };

        /**
         * Texture layer blend modes.
         */
        enum
        {
            LAYERBLENDMODE_NONE         = 0,    /**< The foreground texture covers up the background texture entirely. */
            LAYERBLENDMODE_OVER         = 1,    /**< The foreground texture is applied like a decal to the background. The shape of the decal is determined by the foreground alpha. */
            LAYERBLENDMODE_IN           = 2,    /**< The result is the background texture cut in the shape of the foreground alpha. */
            LAYERBLENDMODE_OUT          = 3,    /**< The result is the opposite of In. It is as if the shape of the foreground alpha has been cut out of the background. */
            LAYERBLENDMODE_ADD          = 4,    /**< The result color is the foreground color added to the background color as if being projected on the background through a slide projector. The result color is then applied over the background color using the foreground alpha to define the opacity of the result. */
            LAYERBLENDMODE_SUBSTRACT    = 5,    /**< The result color is the foreground color subtracted from the background color. The result color is then applied over the background color using the foreground alpha to define the opacity of the result. */
            LAYERBLENDMODE_MULTIPLY     = 6,    /**< The result color is the foreground color multiplied by the background color. The result color is then applied over the background color using the foreground alpha to define the opacity of the result. */
            LAYERBLENDMODE_DIFFERENCE   = 7,    /**< The result color is the difference between the foreground color and the background color. The result color is then applied over the background color using the foreground alpha to define the opacity of the result. */
            LAYERBLENDMODE_LIGHTEN      = 8,    /**< The result color of each pixel is the background color or foreground color, whichever is lighter. The result color is then applied over the background color using the foreground alpha to define the opacity of the result. */
            LAYERBLENDMODE_DARKEN       = 9,    /**< The result color of each pixel is the background color or foreground color, whichever is darker. The result color is then applied over the background color using the foreground alpha to define the opacity of the result. */
            LAYERBLENDMODE_SATURATE     = 10,   /**< The result color is the background color with saturation increased in proportion to the foreground color scaled by foreground alpha. If the foreground color is red, for example, the result color will be the background color with more saturated reds. */
            LAYERBLENDMODE_DESATURATE   = 11,   /**< The result color is the background color with saturation decreased in proportion to the foreground color scaled by foreground alpha. If the foreground color is red, for example, the result color will be the background color with desaturated reds. */
            LAYERBLENDMODE_ILLUMINATE   = 12    /**< The result color is the background color mixed with the foreground color, brighter where the foreground is bright and darker where the foreground is dark. It is as if the foreground texture represents the light falling on the background. The result color is then applied over the background color using the foreground alpha to define the opacity of the result. */
        };

        /**
         * Creation method.
         * The layer type will be set to LAYERTYPE_UNKNOWN, the amount will be set to 1, and the
         * filename will stay uninitialized (empty).
         */
        static StandardMaterialLayer* Create();

        /**
         * Extended creation.
         * @param layerType The layer type.
         * @param fileName The filename, must be without extension or path.
         * @param amount The amount value, which identifies how intense or active the layer is. The normal range is [0..1].
         */
        static StandardMaterialLayer* Create(uint32 layerType, const char* fileName, float amount = 1.0f);

        /**
         * Copy over the data from another layer into this one.
         * @param layer The layer to copy the data from.
         */
        void InitFrom(StandardMaterialLayer* layer);

        /**
         * Get the filename of the layer.
         * @result The filename of the texture of the layer, without path or extension.
         */
        const char* GetFileName() const;

        /**
         * Get the filename of the layer.
         * @result The filename of the texture of the layer, without path or extension.
         */
        const AZStd::string& GetFileNameString() const;

        /**
         * Set the filename of the texture of the layer.
         * @param fileName The filename, must be without extension or path.
         */
        void SetFileName(const char* fileName);

        /**
         * Set the amount value.
         * The amount identifies how active or intense the layer is.
         * The normal range of the value is [0..1].
         * @param amount The amount value.
         */
        void SetAmount(float amount);

        /**
         * Get the amount.
         * The amount identifies how active or intense the layer is.
         * The normal range of the value is [0..1].
         */
        float GetAmount() const;

        /**
         * Get the layer type.
         * @result The layer type ID.
         */
        uint32 GetType() const;

        /**
         * Get the string that is a description or the class name of this material layer type.
         * @result The string containing the description.
         */
        const char* GetTypeString() const;

        /**
         * Set the layer type ID.
         * @param typeID The layer type ID.
         */
        void SetType(uint32 typeID);

        /**
         * Set the blend mode that is used to control how successive layers of textures are combined together.
         * @param layerBlendMode The blend mode used for the current layer.
         */
        void SetBlendMode(unsigned char layerBlendMode);

        /**
         * Get the blend mode that is used to control how successive layers of textures are combined together.
         * @result The blend mode used for the current layer.
         */
        unsigned char GetBlendMode() const;

        /**
         * Get the string that is a description of the material blend mode.
         * @result The string containing the description.
         */
        const char* GetBlendModeString() const;

        /**
         * Set the u offset (horizontal texture shift).
         * @param uOffset The u offset.
         */
        void SetUOffset(float uOffset);

        /**
         * Set the v offset (vertical texture shift).
         * @param vOffset The v offset.
         */
        void SetVOffset(float vOffset);

        /**
         * Set the horizontal tiling factor.
         * @param uTiling The horizontal tiling factor.
         */
        void SetUTiling(float uTiling);

        /**
         * Set the vertical tiling factor.
         * @param vTiling The vertical tiling factor.
         */
        void SetVTiling(float vTiling);

        /**
         * Set the texture rotation.
         * @param rotationRadians The texture rotation in radians.
         */
        void SetRotationRadians(float rotationRadians);

        /**
         * Get the u offset (horizontal texture shift).
         * @result The u offset.
         */
        float GetUOffset() const;

        /**
         * Get the v offset (vertical texture shift).
         * @result The v offset.
         */
        float GetVOffset() const;

        /**
         * Get the horizontal tiling factor.
         * @result The horizontal tiling factor.
         */
        float GetUTiling() const;

        /**
         * Get the vertical tiling factor.
         * @result The vertical tiling factor.
         */
        float GetVTiling() const;

        /**
         * Get the texture rotation.
         * @result The texture rotation in radians.
         */
        float GetRotationRadians() const;

    private:
        uint32          mFileNameID;        /**< The filename of the texture, without extension or path. */
        uint32          mLayerTypeID;       /**< The layer type. See the enum for some possibilities. */
        float           mAmount;            /**< The amount value, between 0 and 1. This can for example represent how intens the layer is. */
        float           mUOffset;           /**< U offset (horizontal texture shift). */
        float           mVOffset;           /**< V offset (vertical texture shift). */
        float           mUTiling;           /**< Horizontal tiling factor. */
        float           mVTiling;           /**< Vertical tiling factor. */
        float           mRotationRadians;   /**< Texture rotation in radians. */
        unsigned char   mBlendMode;         /**< The blend mode is used to control how successive layers of textures are combined together. */

        /**
         * Default constructor.
         * The layer type will be set to LAYERTYPE_UNKNOWN, the amount will be set to 1, and the
         * filename will stay uninitialized (empty).
         */
        StandardMaterialLayer();

        /**
         * Extended constructor.
         * @param layerType The layer type.
         * @param fileName The filename, must be without extension or path.
         * @param amount The amount value, which identifies how intense or active the layer is. The normal range is [0..1].
         */
        StandardMaterialLayer(uint32 layerType, const char* fileName, float amount = 1.0f);

        /**
         * The destructor.
         */
        ~StandardMaterialLayer();
    };



    /**
     * The material class.
     * This class describes the material properties of renderable surfaces.
     * Every material can have a set of material layers, which contain the texture information.
     */
    class EMFX_API StandardMaterial
        : public Material
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

        /**
         * Creation method.
         * @param name The name of the material.
         */
        static StandardMaterial* Create(const char* name);

        /**
         * Set the ambient color.
         * @param ambient The ambient color.
         */
        void SetAmbient(const MCore::RGBAColor& ambient);

        /**
         * Set the diffuse color.
         * @param diffuse The diffuse color.
         */
        void SetDiffuse(const MCore::RGBAColor& diffuse);

        /**
         * Set the specular color.
         * @param specular The specular color.
         */
        void SetSpecular(const MCore::RGBAColor& specular);

        /**
         * Set the self illumination color.
         * @param emissive The self illumination color.
         */
        void SetEmissive(const MCore::RGBAColor& emissive);

        /**
         * Set the shine.
         * @param shine The shine.
         */
        void SetShine(float shine);

        /**
         * Set the shine strength.
         * @param shineStrength The shine strength.
         */
        void SetShineStrength(float shineStrength);

        /**
         * Set the opacity amount (opacity) [1.0=full opac, 0.0=full transparent].
         * @param opacity The opacity amount.
         */
        void SetOpacity(float opacity);

        /**
         * Set the index of refraction.
         * @param ior The index of refraction.
         */
        void SetIOR(float ior);

        /**
         * Set double sided flag.
         * @param doubleSided True if it is double sided, false if not.
         */
        void SetDoubleSided(bool doubleSided);

        /**
         * Set the wireframe flag.
         * @param wireFrame True if wireframe rendering is enabled, false if not.
         */
        void SetWireFrame(bool wireFrame);

        /**
         * Get the ambient color.
         * @result The ambient color.
         */
        const MCore::RGBAColor& GetAmbient() const;

        /**
         * Get the diffuse color.
         * @result The diffuse color.
         */
        const MCore::RGBAColor& GetDiffuse() const;

        /**
         * Get the specular color.
         * @result The specular color.
         */
        const MCore::RGBAColor& GetSpecular() const;

        /**
         * Get the self illumination color.
         * @result The self illumination color.
         */
        const MCore::RGBAColor& GetEmissive() const;

        /**
         * Get the shine.
         * @result The shine.
         */
        float GetShine() const;

        /**
         * Get the shine strength.
         * @result The shine strength.
         */
        float GetShineStrength() const;

        /**
         * Get the opacity amount [1.0=full opac, 0.0=full transparent].
         * @result The opacity amount.
         */
        float GetOpacity() const;

        /**
         * Get the index of refraction.
         * @result The index of refraction.
         */
        float GetIOR() const;

        /**
         * Get double sided flag.
         * @result True if it is double sided, false if not.
         */
        bool GetDoubleSided() const;

        /**
         * Get the wireframe flag.
         * @result True if wireframe rendering is enabled, false if not.
         */
        bool GetWireFrame() const;

        /**
         * Pre-allocate space for a given amount of material layers.
         * This does not influence the return value of GetNumLayers().
         * @param numLayers The number of layers to pre-allocate space for.
         */
        void ReserveLayers(uint32 numLayers);

        /**
         * Add a given layer to this material.
         * @param layer The layer to add.
         */
        StandardMaterialLayer* AddLayer(StandardMaterialLayer* layer);

        /**
         * Get the number of texture layers in this material.
         * @result The number of layers.
         */
        uint32 GetNumLayers() const;

        /**
         * Get a specific layer.
         * @param nr The material layer number to get.
         * @result A pointer to the material layer.
         */
        StandardMaterialLayer* GetLayer(uint32 nr);

        /**
         * Remove a specified material layer (also deletes it from memory).
         * @param nr The material layer number to remove.
         * @param delFromMem Set to true if it should be deleted from memory as well.
         */
        void RemoveLayer(uint32 nr, bool delFromMem = true);

        /**
         * Removes all material layers from this material (includes deletion from memory).
         */
        void RemoveAllLayers();

        void RemoveLayer(StandardMaterialLayer* layer, bool delFromMem = true);

        /**
         * Find the layer number which is of the given type.
         * If you for example want to search for a diffuse layer, you make a call like:
         *
         * uint32 layerNumber = material->FindLayer( StandardMaterialLayer::LAYERTYPE_DIFFUSE );
         *
         * This will return a value the layer number, which can be accessed with the GetLayer(layerNumber) method.
         * A value of MCORE_INVALIDINDEX32 will be returned in case no layer of the specified type could be found.
         * @param layerType The layer type you want to search on, for a list of valid types, see the enum inside StandardMaterialLayer.
         * @result Returns the layer number or MCORE_INVALIDINDEX32 when it could not be found.
         */
        uint32 FindLayer(uint32 layerType) const;

        /**
         * Creates a clone of the material, including it's layers.
         * @result A pointer to an exact clone (copy) of the material.
         */
        Material* Clone() const override;

        /**
         * Get the unique type ID of this type of material.
         * @result The unique ID of this material type.
         */
        uint32 GetType() const override                 { return TYPE_ID; }

        /**
         * Get the string that is a description or the class name of this material type.
         * @result The string containing the description or class name.
         */
        const char* GetTypeString() const override      { return "StandardMaterial"; }


    protected:
        MCore::Array< StandardMaterialLayer* > mLayers; /**< StandardMaterial layers. */
        MCore::RGBAColor    mAmbient;           /**< Ambient color. */
        MCore::RGBAColor    mDiffuse;           /**< Diffuse color. */
        MCore::RGBAColor    mSpecular;          /**< Specular color. */
        MCore::RGBAColor    mEmissive;          /**< Self illumination color. */
        float               mShine;             /**< The shine value, from the phong component (the power). */
        float               mShineStrength;     /**< Shine strength. */
        float               mOpacity;           /**< The opacity amount [1.0=full opac, 0.0=full transparent]. */
        float               mIOR;               /**< Index of refraction. */
        bool                mDoubleSided;       /**< Double sided?. */
        bool                mWireFrame;         /**< Render in wireframe?. */

        /**
         * Constructor.
         * @param name The name of the material.
         */
        StandardMaterial(const char* name);

        /**
         * Destructor.
         */
        virtual ~StandardMaterial();
    };
} // namespace EMotionFX

