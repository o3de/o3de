################################
ociolutimage --generate --cubesize 16 --output C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr --op pre-grading --shaper Log2-48nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-48nits_16_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr --op pre-grading --shaper Log2-1000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-1000nits_16_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr --op pre-grading --shaper Log2-2000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-2000nits_16_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr --op pre-grading --shaper Log2-4000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-4000nits_16_LUT.exr


################################
ociolutimage --generate --cubesize 32 --output C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr --op pre-grading --shaper Log2-48nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-48nits_32_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr --op pre-grading --shaper Log2-1000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-1000nits_32_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr --op pre-grading --shaper Log2-2000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-2000nits_32_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr --op pre-grading --shaper Log2-4000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-4000nits_32_LUT.exr


################################
# LUT size 64 broken output!!!
ociolutimage --generate --cubesize 64 --maxwidth 4096 --output C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_64_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_64_LUT.exr --op pre-grading --shaper Log2-48nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-48nits_64_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_64_LUT.exr --op pre-grading --shaper Log2-1000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-1000nits_64_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_64_LUT.exr --op pre-grading --shaper Log2-2000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-2000nits_64_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_64_LUT.exr --op pre-grading --shaper Log2-4000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-4000nits_64_LUT.exr

