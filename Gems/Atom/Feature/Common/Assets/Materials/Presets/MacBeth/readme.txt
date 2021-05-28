We used the sRGB values from 'Table 2: R'G'B coordinates of the ColorChecker' from this PDF:
http://www.babelcolor.com/index_htm_files/RGB%20Coordinates%20of%20the%20Macbeth%20ColorChecker.pdf

Specifically the second table labeled 'BabelColor Avg.' as this is a spectral average of 20 charts.

Those are the values typed into the color picker in sRGB swatches (stored in the material as linear RGB values)

Note: the colors displayed in that pdf table (on the left) do not seem to represent the actual sRGB values of the table (I found that odd)

since we are entering 1...255 values, instead of normalized 0...1 values, or 16-bit values, there is actualy some minor rounding errors.

The texture: ColorChecker_sRGB_from_Lab_16bit_AfterNov2014.tif is 16-bit color
But photoshop will quantize to 15+ bit color
Also photoshops color picker doesn't allow you to enter 16 bit color

So the avg sRGB values in the above pdf,
the values in the 16-bit macbeth texture,
and values color picked are slightly different
(but may or may not be noticable.)

That macbeth chart came from this more comprehensive site (which covers a lot of the variance):
http://www.babelcolor.com/colorchecker-2.htm#CCP2_images

The texture was converted to 1024x1024 based on the sRGB image 16-bit TIF in the section:
From X-Rite L*a*b* D50 (formulations AFTER Nov. 2014)

So, for now, that 16-bit .tif should be the most accurate - and in the future we may be able to enter more precise numeric values.  This is after all meant to be a baseline and validate raw color versus color from texture.

Also i note, that the color swatch versus color texture material variants have a slight visual difference I am unable to
currently reconcile - could be a number of things, like a color transform somewhere, mathematical rounding error, etc.

Will need to come back and figure out where it is exactly.

Additional information about colors are here, along with what seems to be a pretty (not entirely) accurate set of macbeth charts for sRGB and ACES AP0 color spaces (they are crappy jpeg, so the AP0 isn't even a linear floating point image.)

http://www.nukepedia.com/gizmos/draw/x-rite-colorchecker-classic-2005-gretagmacbeth

Note: picking against the visual macbeth chart image versus the values in the original pdf there is a slight difference. I have leaned on the side of the spectral average in the original pdf.
