Common 
======

Here lives all the code that is common to the DX12 and VK backends

### Directory structure:

Note that it it tries to follow the directory structure of the DX12/VK backends 

* **base**: 
    * ShaderCompiler: generates a hash of shader code
* **GLTF**: 
    * GLTFStructures: all the structures needed by the GLTF specs
    * GLTFCommon: Loads, animates and transform the scene. The DX12/VK rendering passes will pick the data they need from this class.
* **Misc**
    * Camera: The typical camera code
    * DDSLoader: loads DDS imges
    * WICLoader: loads other types of images(PNG,JPGs,...) and can generate mip maps
    * WirePrimitives
    

