################################
# create identity LUT
ociolutimage --generate --cubesize 16 --output C:\Depot\o3de\Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\linear_16_LUT.exr

NOTE:	ociolutimage is a OpenColorIO (ocio) app.  O3DE does NOT currently build these, however you can build them yourself.
		for example, we used vcpkg and cmake gui to build them locally.
		we intend to add a fully built integration of ocio in the future.
		for now, we have provided all of the LUTs below so that most users will not need the ocio tools.

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
		"ColorGrading\Resources\TestData\Photoshop\Log2-48nits\framebuffer_ACEScg_to_sRGB_icc.exr"
		^ open in photoshop, color proof with the above .icc
		  the image should proof such that it looks the same as camera in O3DE viewport
		  with the Displaymapper set to ACES

	3. Composite this base LUT in a layer on top:
		"Gems\Atom\Feature\Common\Tools\ColorGrading\Resources\LUTs\base_Log2-48nits_32_LUT.exr"
		^ this is a pre-shaped 32x32x32 LUT we generated above

	4. Color Grade using adjustment layers (and hope for the best, some photoshop grades may cause values to clip)

	5. Select/crop just the LUT, 'Copy Merged' into a new document, save into a folder such as:
		"ColorGrading\Resources\TestData\Photoshop\Log2-48nits\i_shaped\test_hue-sat_32_LUT.exr"
			^ this lut is post-grade BUT still shaped (need to inv shape back to normailzed 0-1 values)

	6. Use the provided cmd/python tools, we need to inverse shape:

		Run: "Gems\Atom\Feature\Common\Tools\ColorGrading\cmdline\CMD_ColorGradingTools.bat"

		C:\Depot\o3de-engine\Gems\Atom\Feature\Common\Tools\ColorGrading>>

		python %DCCSI_COLORGRADING_SCRIPTS%\lut_helper.py --i "Resources\TestData\TestData\Photoshop\Log2-48nits\i_shaped\test_hue-sat_32_LUT.exr" --op post-grading --shaper Log2-48nits --o Resources\TestData\Photoshop\Log2-48nits\o_invShaped\test_hue-sat_32_LUT -l -a

		^ this will inv-shaped LUT (3DL's are normalized in 0-1 space, values may clip!!!)
		^ -e generates LUT as .exr
		^ -l generates a .3DL version
		^ -a generates a .azasset version <-- this copies/moves to an asset folder

Using LUTs In-engine:
		Only the <LUT NAME>.azasset needs to be copied to a O3DE project assets folder to use in engine, example:
			"C:\Depot\o3de-engine\Gems\Atom\Feature\Common\Assets\ColorGrading\TestData\Photoshop\inv-Log2-48nits\test_hue-sat_32_LUT.azasset"
			^ this is just an example path within the engine folder

		The <LUT NAME>.azasset can be loaded into a 'Look Modification Component'

		If there are multiple properly configured 'Look Modification Components' you can blend LUTs/Looks (in world space, in a cinematic, etc.)

		By default LUTs utilize linear sampling, but you can use the following CVAR to improve quality for 'any difficult lut':
			r_lutSampleQuality=0, linear
			r_lutSampleQuality=1, b-spline 7 taps 
			r_lutSampleQuality=2, b-spline 19 taps (highest quality)