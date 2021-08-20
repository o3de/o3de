################################
# create identity LUT
ociolutimage --generate --cubesize 16 --output C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr


################################
# create base 16x16x16 LUT shaped for grading in 48-nits (LDR), in 1000-nits (HDR), 2000nits, 4000-nits
python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr --op pre-grading --shaper Log2-48nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-48nits_16_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr --op pre-grading --shaper Log2-1000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-1000nits_16_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr --op pre-grading --shaper Log2-2000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-2000nits_16_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr --op pre-grading --shaper Log2-4000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-4000nits_16_LUT.exr


################################
# create base 32x32x32 LUT shaped for grading in 48-nits (LDR), in 1000-nits (HDR), 2000nits, 4000-nits
ociolutimage --generate --cubesize 32 --output C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr --op pre-grading --shaper Log2-48nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-48nits_32_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr --op pre-grading --shaper Log2-1000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-1000nits_32_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr --op pre-grading --shaper Log2-2000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-2000nits_32_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_32_LUT.exr --op pre-grading --shaper Log2-4000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-4000nits_32_LUT.exr


################################
# create base 64x64x64 LUT shaped for grading in 48-nits (LDR), in 1000-nits (HDR), 2000nits, 4000-nits
# LUT size 64 broken output without --maxwidth 4096!!!
ociolutimage --generate --cubesize 64 --maxwidth 4096 --output C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_64_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_64_LUT.exr --op pre-grading --shaper Log2-48nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-48nits_64_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_64_LUT.exr --op pre-grading --shaper Log2-1000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-1000nits_64_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_64_LUT.exr --op pre-grading --shaper Log2-2000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-2000nits_64_LUT.exr

python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_64_LUT.exr --op pre-grading --shaper Log2-4000nits --o C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-4000nits_64_LUT.exr


################################
# Photoshop, Basics
Examples: 	C:\Depot\o3de-engine\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\TestData\Photoshop\
			HDR_in_LDR_Log2-48nits\LUT_PreShaped_Composite\*

	1. This must be installed (righ-click) before launching photoshop:
		Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\from_ACEScg_to_sRGB.icc
		^ this is to be used as 'custom color proof' (ACES view transform)
	2. This is the O3DE 'displaymapper passthrough' capture, re-saved as a .exr:
		"HDR_in_LDR_Log2-48nits\LUT_PreShaped_Composite\framebuffer_ACEScg_to_sRGB_icc.exr"
		^ open in photoshop, color proof with the above .icc
		  the image should proof such that it looks the same as camer in O3DE viewport
		  with the Displaymapper set to ACES
	3. Composite this base LUT in a layer on top:
		"Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-48nits_32_LUT.exr"
		^ this is a pre-shaped LUT we generated above
	4. Color Grade using adjustment layers (and hope for the best)
	5. Select/crop just the LUT, 'Copy Merged' into a new document, save:
		"HDR_in_LDR_Log2-48nits\LUT_PreShaped_Composite\shaper-Log2-48nits_hue-sat_post-grade_32_LUT.exr"
		^ this lut is post-grade BUT still shaped
	6. Use the provided cmd/python tools, we need to inverse shape:

		Run: "Gems\Atom\Feature\Common\Tools\ColorGrading\cmdline\CMD_ColorGradinTools.bat"

		C:\Depot\o3de-engine\Gems\Atom\Feature\Common\Tools\ColorGrading>>

		python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i "Resources\TestData\Photoshop\HDR_in_LDR_Log2-48nits\LUT_PreShaped_Composite\shaper-Log2-48nits_hue-sat_post-grade_32_LUT.exr" --op post-grading --shaper Log2-48nits --o Resources\TestData\Photoshop\HDR_in_LDR_Log2-48nits\LUT_PreShaped_Composite\hue-sat_inv-Log2-48nits_32_LUT.exr -l -a

		^ this will inv-shape the LUT (3DL's are normalized, values may clip!)
		^ generates LUT as .exr
		^ -l generates a .3DL version
		^ -a generates a .azasset version <-- this copies/moves to an asset folder
