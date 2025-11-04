_iblskyboxcm

The input HDRI cubemap generates a skybox, specular IBL, and diffuse IBL output cubemaps. This is the least complex of all use-cases, and is useful when the input HDRI image does require any manual authoring to adjust the amount of light for the IBL cubemaps.

Example:

Source cubemap: CloudyDaySkybox_iblskyboxcm.dds (4096x4096)

Outputs:

CloudyDaySkybox_iblskyboxcm.dds.streamingimage - Skybox cubemap (4096x4096)
CloudyDaySkybox_iblskyboxcm_iblspecular.dds.streamingimage - Specular IBL cubemap (256x256)
CloudyDaySkybox_iblskyboxcm_ibldiffuse.dds.streamingimage - Diffuse IBL cubemap (128x128)

_iblglobalcm

The input cubemap generates specular and diffuse IBL cubemaps only. This is useful when the HDRI skybox image needs to be manually adjusted to reduce, or increase, the amount of light supplied by the IBL cubemaps. In this scenario, the source HDRI cubemap file is edited then saved with the _iblglobalcm filemask. The original untouched image is renamed with the _skycm file mask (see next item), and is used for the skybox image itself.

Manual editing is typically required when the HDRI image contains the Sun, which must be adjusted by an artist to reduce the amount of irradiance from the IBL when combined with the directional light in a scene.

This filemask is also used by the global environment probe, if placed in the scene, to automatically generate the specular and diffuse IBL cubemaps. The global environment probe is required when using a physical sky.

Example:

Source cubemap: SunnyDaySkybox_iblglobalcm.dds (4096x4096)

Outputs:

SunnyDaySkybox_iblglobalcm_iblspecular.dds.streamingimage - Specular IBL cubemap (256x256)
SunnyDaySkybox_iblglobalcm_ibldiffuse.dds.streamingimage - Diffuse IBL cubemap (128x128)

_skyboxcm

The input cubemap generates a skybox cubemap only. This is typically used in combination with the _iblglobalcm file mask when a separate image is used to generate the IBL cubemaps.

Example:

Source cubemap: SunnyDaySkybox_skyboxcm.dds (4096x4096)

Output:

SunnyDaySkybox_skyboxcm.dds.streamingimage - Skybox cubemap (4096x4096)

_iblspecularcm

The input cubemap generates a specular IBL cubemap only. The main use-case for this file mask is by the Editor when generating specular reflection probe cubemaps. It can also be used to manually specify a global IBL specular cubemap when it needs to be authored separately from the global diffuse IBL cubemap.

Example:

Source cubemap: SunnyDaySkybox_iblspecularcm.dds (4096x4096)

Output:

SunnyDaySkybox_iblspecularcm.dds.streamingimage - Specular IBL cubemap (256x256)

_ibldiffusecm

The input cubemap generates a diffuse IBL cubemap only. This is used to manually specify a global IBL diffuse cubemap when it needs to be authored separately from the global IBL specular cubemap.

Example:

Source cubemap: SunnyDaySkybox_ibldiffusecm.dds (4096x4096)

Output:

SunnyDaySkybox_ibldiffusecm.dds.streamingimage - Diffuse IBL cubemap (128x128)

_ccm

	
The input cubemap is pre-convolved and passed through untouched, including all mips. This is used in rare cases when the user wants to convolve a specular or diffuse IBL cubemap with an external tool (like IBLbaker) and then use the results in Lumberyard as a global or local IBL cubemap.

_cm

Legacy support for the _cm file mask. It is equivalent to the _iblglobalcm file mask described above.
