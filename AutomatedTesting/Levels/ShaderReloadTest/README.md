# About ShaderReloadTest
This level, along with the companion python script named `shader_reload_test.py` were created with the only purpose of validating that the engine can properly react to changes to Shader Assets.  
  
Historically, OnAssetReloaded() event was not being properly processed by the AZ::RPI::Shader class. Developers would find themselves in the situation of modifying and saving a shader file several times to make sure the rendered viewport would represent the correct state of the Shader files.  
  
The script `shader_reload_test.py` is expected to be executed manually. This is not part of the Unit Tests, nor should be executed
as part of the Automation Tests.  
  
# How to use
1- In the Editor, open this level: `ShaderReloadTest`.  
2- After the level is loaded, execute the following command in the console:  
```
pyRunFile C:\GIT\o3de\AutomatedTesting\Levels\ShaderReloadTest\shader_reload_test.py
```
Depending on where the engine was installed, you may change the begginning of the absolute path shown above: `C:\GIT\o3de\AutomatedTesting\...`
  
By default, the previous command will run only ONE Shader modification cycle, if you want to run 10 test cycles you can try the `-i`(`--iterations`) argument:
```
pyRunFile C:\GIT\o3de\AutomatedTesting\Levels\ShaderReloadTest\shader_reload_test.py -i 10
```
  
# For Power Users
To get a list of all the options this script supports use the `-h`(`--help`) argument:  
```
pyRunFile C:\GIT\o3de\AutomatedTesting\Levels\ShaderReloadTest\shader_reload_test.py --help
[CONSOLE] Executing console command 'pyRunFile C:\GIT\o3de\AutomatedTesting\Levels\ShaderReloadTest\shader_reload_test.py --help'
(python_test) - usage: shader_reload_test.py [-h] [-i ITERATIONS]
                             [--screen_update_wait_time SCREEN_UPDATE_WAIT_TIME]
                             [--capture_count_on_failure CAPTURE_COUNT_ON_FAILURE]
                             [-p SCREENSHOT_IMAGE_PATH]
                             [-q QUICK_SHADER_OVERWRITE_COUNT]
                             [-w QUICK_SHADER_OVERWRITE_WAIT]
                             [-c CAMERA_ENTITY_NAME]

Records several frames of pass attachments as image files.

options:
  -h, --help            show this help message and exit
  -i ITERATIONS, --iterations ITERATIONS
                        How many times the Shader should be modified and the
                        screen pixel validated.
  --screen_update_wait_time SCREEN_UPDATE_WAIT_TIME
                        Minimum time to wait after modifying the shader and
                        taking the screen snapshot to validate color output.
  --capture_count_on_failure CAPTURE_COUNT_ON_FAILURE
                        How many times the screen should be recaptured if the
                        pixel output failes.
  -p SCREENSHOT_IMAGE_PATH, --screenshot_image_path SCREENSHOT_IMAGE_PATH
                        Absolute path of the file where the screenshot will be
                        written to. Must include image extensions 'ppm',
                        'png', 'bmp', 'tif'. By default a temporary png path
                        will be created
  -q QUICK_SHADER_OVERWRITE_COUNT, --quick_shader_overwrite_count QUICK_SHADER_OVERWRITE_COUNT
                        How many times the shader should be overwritten before
                        capturing the screenshot. This simulates real life
                        cases where a shader file is updated and saved to the
                        file system several times consecutively
  -w QUICK_SHADER_OVERWRITE_WAIT, --quick_shader_overwrite_wait QUICK_SHADER_OVERWRITE_WAIT
                        Minimum time to wait in between quick shader
                        overwrites.
  -c CAMERA_ENTITY_NAME, --camera_entity_name CAMERA_ENTITY_NAME
                        Name of the entity that contains a Camera Component.
                        If found, the Editor camera will be set to it before
                        starting the test.
```  
  
# Technical Details
The level named `ShaderReloadTest` looks like the `Default Atom` level, but contains Three aditional entities, all of them working together using the Render To Texture technique. The Texture asset is called `billboard_visualize_rtt.attimage`:  

1- `Billboard` Entity: This entity presents in a flat Mesh the rendering result of a custom Render Pipeline named `SimpleMeshPipeline`. The `Base Color` Texture in its material (`billboard_visualize_rtt.material`) points to `billboard_visualize_rtt.attimage`.  
2- `RTT Camera` Entity: This entity holds a Camera that Renders To Texture, it uses the ustom Render Pipeline named `SimpleMeshPipeline` to render into the Texture Asset named `billboard_visualize_rtt.attimage`.  
3- `Shader Ball Simple Pipeline` Entity: This entity has the `Shader Ball` mesh with a custom material named `simple_mesh.material`. Any object that uses this Material will be visible under the  `SimpleMeshPipeline`, with a simple color Red, Green or Blue.  
  
## Why A Custom Render Pipeline For This Test?
The idea is to be able to change Shader code and wait the least amount of time for the Asset Processor to complete the compilation. When you modify a Shader file related to the Main Render Pipeline it may trigger a chain reaction of around 500 files to be reprocessed. On the other hand, by having a custom Render Pipeline, along with a custom material type (`simple_mesh.materialtype`) which references only one Shader (`SimpleMesh.shader`) it only take a less than 3 seconds to compile the Shader and see color updates in the screen.  
  
## Why All Assets Are Placed under `Levels\ShaderReloadTest`?
Because it makes the whole test suite self contained and easy to port to other Game Projects.  
The only asset related to this Level, that was added outside of this folder is `Assets/Passes/AutomatedTesting/AutoLoadPassTemplates.azsset`. In the future, if more custom passes and/or pipelines need to be added by other Test Suites, they can be added to this file.  
