-------------------------------------------------------------------------------
--	CryArtistTools.ms
--	Version 2.2 External
--	by Christopher Evans
-------------------------------------------------------------------------------

--	Globals
-------------------------------------------------------------------------------

global CryArtistTools

crytools.maxDirTxt = (getdir #maxroot)

-------------------------------------------------------------------
-----------Update "CGF Metadata Manager" Function---------
-------------------------------------------------------------------
udpInARR = #()
testFor = #()
fn udpGetFromSel =
(
	likeMass = false
	likeDensity = false
	multipleObj = false
	AllMass = false
	AllDens = false
	AllDensity = false
	AllLimit = false
	AllTwist = false
	AllBend = false
	AllPull = false
	AllPush = false
	AllShift = false
	LikeLimit = false
	LikeTwist = false
	LikeBend = false
	LikePull = false
	LikePush = false
	LikeShift = false
	
	CryArtistTools.rollouts[3].objScale.value = 100
	
	if $ == undefined then
	(
		CryArtistTools.rollouts[3].isMain.checked = false
		CryArtistTools.rollouts[3].isRemain.checked = false
		CryArtistTools.rollouts[3].updateLBL.text = "Nothing selected"
		CryArtistTools.rollouts[3].UpdateUDP.enabled = false
		CryArtistTools.rollouts[3].udpSetToSel.enabled = false
		return undefined
	)
	
	if CryArtistTools != undefined then
	(	
		CryArtistTools.rollouts[3].UpdateUDP.enabled = false
		CryArtistTools.rollouts[3].udpSetToSel.enabled = true	
		if selection.count > 1 then multipleObj = true
		if selection.count == 1 then
		(
			case selection[1].name of
			(
				"Main": (CryArtistTools.rollouts[3].isMain.checked = true)
				"Remain": (CryArtistTools.rollouts[3].isRemain.checked = true)
			)
			if selection[1].name != "Main" and selection[1].name!="Remain" then
			(
				CryArtistTools.rollouts[3].isMain.checked = false
				CryArtistTools.rollouts[3].isRemain.checked = false
			)
		)
		LimitArr = #()
		TwistArr = #()
		BendArr = #()
		PullArr = #()
		PushArr = #()
		ShiftArr = #()
		
		MassArr = #()
		DensArr = #()
		for obj in selection do
		(
			Temp = #()
			udpInARR = crytools.inFromUDP obj
			if udpInARR.count == 0 then
			(
				CryArtistTools.rollouts[3].updateLBL.text = "Object has no UDP data"
				if multipleObj == true then
				(
					CryArtistTools.rollouts[3].updateLBL.text = "Objects have no UDP data"
				)
			)
			else
			(
				CryArtistTools.rollouts[3].updateLBL.text = "UDP data loaded"
				IsMass = (filterstring udpInARR[1] " =")
				if IsMass[1] == "mass" then
				(
					append MassArr obj
				)
				else
				(
					if IsMass[1] == "density" then
					(
						append DensArr obj
					)
				)
			)
			
			for i = 1 to udpInArr.count do
			(
				testFor = (filterstring udpInARR[i] " =")[1]
				append Temp testFor
			)
			if (findItem Temp "limit") != 0 then append LimitArr obj
			if (findItem Temp "twist") != 0 then append TwistArr obj
			if (findItem Temp "bend") != 0 then append BendArr obj
			if (findItem Temp "pull") != 0 then append PullArr obj
			if (findItem Temp "push") != 0 then append PushArr obj
			if (findItem Temp "shift") != 0 then append ShiftArr obj
		)
		
		if multipleObj == true then
		(	
			CryArtistTools.rollouts[3].isMain.checked = false
			CryArtistTools.rollouts[3].isRemain.checked = false
			if MassArr.count != 0 then
			(
				if MassArr.count == selection.count then AllMass = true else AllMass = false
			)
			
			if DensArr.count != 0 then
			(
				if DensArr.count == selection.count then AllDens = true else AllDens = false
			)
			
			if LimitArr.count != 0 then
			(
				if LimitArr.count == selection.count then AllLimit = true else AllLimit = false
			)

			if TwistArr.count != 0 then
			(
				if TwistArr.count == selection.count then AllTwist = true else AllTwist = false
			)

			if BendArr.count != 0 then
			(
				if BendArr.count == selection.count then AllBend = true else AllBend = false
			)
			
			if PullArr.count != 0 then
			(
				if PullArr.count == selection.count then AllPull = true else AllPull = false
			)
			
			if PushArr.count != 0 then
			(
				if PushArr.count == selection.count then AllPush = true else AllPush = false
			)
			
			if ShiftArr.count != 0 then
			(
				if ShiftArr.count == selection.count then AllShift = true else AllShift = false
			)
		)
		
		testForMass = (crytools.getUDP selection[1] "mass")
		for i = 2 to selection.count do
		(
			if (crytools.getUDP selection[i] "mass") != testForMass then likeMass = false else likeMass = true
		)
		
		testForDensity = (crytools.getUDP selection[1] "density")
		for i = 2 to selection.count do
		(
			if (crytools.getUDP selection[i] "density") != testForDensity then likeDensity = false else likeDensity = true
		)
		
		testForLimit = (crytools.getUDP selection[1] "limit")
		for i = 2 to selection.count do
		(
			if (crytools.getUDP selection[i] "limit") != testForLimit then likeLimit = false else likeLimit = true
		)
		
		testForTwist = (crytools.getUDP selection[1] "twist")
		for i = 2 to selection.count do
		(
			if (crytools.getUDP selection[i] "twist") != testForTwist then likeTwist = false else likeTwist = true
		)
		
		testForBend = (crytools.getUDP selection[1] "bend")
		for i = 2 to selection.count do
		(
			if (crytools.getUDP selection[i] "bend") != testForBend then likeBend = false else likeBend = true
		)
		
		testForPull = (crytools.getUDP selection[1] "pull")
		for i = 2 to selection.count do
		(
			if (crytools.getUDP selection[i] "pull") != testForPull then likePull = false else likePull = true
		)
		
		testForPush = (crytools.getUDP selection[1] "push")
		for i = 2 to selection.count do
		(
			if (crytools.getUDP selection[i] "push") != testForPush then likePush = false else likePush = true
		)
		
		testForShift = (crytools.getUDP selection[1] "shift")
		for i = 2 to selection.count do
		(
			if (crytools.getUDP selection[i] "shift") != testForShift then likeShift = false else likeShift = true
		)
		
		-- reset
		CryArtistTools.rollouts[3].cylinder1.checked = false
		CryArtistTools.rollouts[3].box1.checked = false
		CryArtistTools.rollouts[3].capsule1.checked = false
		CryArtistTools.rollouts[3].sphere1.checked = false
		
		CryArtistTools.rollouts[3].useMass.checked = false
		CryArtistTools.rollouts[3].objMass.value = 0
		CryArtistTools.rollouts[3].objMass.enabled = false
		CryArtistTools.rollouts[3].useDensity.checked = false
		CryArtistTools.rollouts[3].objDensity.value = 0
		CryArtistTools.rollouts[3].objDensity.enabled = false
		CryArtistTools.rollouts[3].objScale.enabled = false
		
		CryArtistTools.rollouts[3].jLimit.enabled = false
		CryArtistTools.rollouts[3].jLimit.checked = false
		CryArtistTools.rollouts[3].jTwist.enabled = false
		CryArtistTools.rollouts[3].jTwist.checked = false
		CryArtistTools.rollouts[3].jBend.enabled = false
		CryArtistTools.rollouts[3].jBend.checked = false
		CryArtistTools.rollouts[3].jPull.enabled = false
		CryArtistTools.rollouts[3].jPull.checked = false
		CryArtistTools.rollouts[3].jPush.enabled = false
		CryArtistTools.rollouts[3].jPush.checked = false
		CryArtistTools.rollouts[3].jShift.enabled = false
		CryArtistTools.rollouts[3].jShift.checked = false
		CryArtistTools.rollouts[3].jScale.enabled = false
		CryArtistTools.rollouts[3].jScale.checked = false
		CryArtistTools.rollouts[3].jointScale.enabled = false
		CryArtistTools.rollouts[3].jointScale.value = 100
		
		CryArtistTools.rollouts[3].usejoint.checked = false
		CryArtistTools.rollouts[3].jointLimit.value = 0
		CryArtistTools.rollouts[3].jointLimit.enabled = false
		CryArtistTools.rollouts[3].jointTwist.value = 0
		CryArtistTools.rollouts[3].jointTwist.enabled = false
		CryArtistTools.rollouts[3].jointBend.value = 0
		CryArtistTools.rollouts[3].jointBend.enabled = false
		CryArtistTools.rollouts[3].jointPull.value =0
		CryArtistTools.rollouts[3].jointPull.enabled = false
		CryArtistTools.rollouts[3].jointPush.value = 0
		CryArtistTools.rollouts[3].jointPush.enabled = false
		CryArtistTools.rollouts[3].jointShift.value = 0
		CryArtistTools.rollouts[3].jointShift.enabled = false
		
		CryArtistTools.rollouts[3].isEntity.checked = false
		CryArtistTools.rollouts[3].xrot.checked = false
		CryArtistTools.rollouts[3].yrot.checked = false
		CryArtistTools.rollouts[3].zrot.checked = false
		CryArtistTools.rollouts[3].sizevar.value = 0
		CryArtistTools.rollouts[3].GenericCount.value = 0

		
		--CryArtistTools.rollouts[3].ispickable.checked = false
		
		--lowercase inputs
		for i = 1 to udpInARR.count do
		(
			testFor = (filterstring udpInARR[i] " =")
			
			wordFilter = (crytools.forceLowerCase testFor[1])
			
			jointsPresent = false
			
			case wordFilter of
			(
				"mass":
				(
					CryArtistTools.rollouts[3].useMass.checked = true
					CryArtistTools.rollouts[3].objMass.enabled = true
					CryArtistTools.rollouts[3].objMass.value = (testFor[2] as float)
					CryArtistTools.rollouts[3].objScale.enabled = true
					if multipleObj == true then
					(
						if AllMass == true then
						(
							if CryArtistTools.rollouts[3].objMass.value != 0 then
							(
								CryArtistTools.rollouts[3].UpdateUDP.enabled = true
								CryArtistTools.rollouts[3].udpSetToSel.enabled = false
							)
							else
							(
								CryArtistTools.rollouts[3].udpSetToSel.enabled = true
							)
							
							if likeMass == true then
							(
								CryArtistTools.rollouts[3].updateLBL.text = "Objects have same mass"
								--print "Selected objects have like mass"
							)
							else
							(
								CryArtistTools.rollouts[3].updateLBL.text = "Multiple objects: different mass"
								--print "Multiple objects: different Mass"
								CryArtistTools.rollouts[3].useMass.checked = false
								CryArtistTools.rollouts[3].objMass.enabled = false
								CryArtistTools.rollouts[3].objScale.enabled = true
								CryArtistTools.rollouts[3].objMass.value = 0
							
							)
						)
						else
						(
							CryArtistTools.rollouts[3].updateLBL.text = "Multiple objects: Data mismatch"
							CryArtistTools.rollouts[3].useMass.checked = false
							CryArtistTools.rollouts[3].objMass.enabled = false
							CryArtistTools.rollouts[3].objScale.enabled = false
							CryArtistTools.rollouts[3].objMass.value = 0
						)
					)
				)
				"density":
				(
					CryArtistTools.rollouts[3].useDensity.checked = true
					CryArtistTools.rollouts[3].objDensity.enabled = true
					CryArtistTools.rollouts[3].objDensity.value = (testFor[2] as float)
					CryArtistTools.rollouts[3].objScale.enabled = true
					
					if multipleObj == true then
					(
						if AllDens == true then
						(
							if CryArtistTools.rollouts[3].objDensity.value != 0 then
							(
								CryArtistTools.rollouts[3].UpdateUDP.enabled = true
								CryArtistTools.rollouts[3].udpSetToSel.enabled = false
							)
							else
							(
								CryArtistTools.rollouts[3].udpSetToSel.enabled = true
							)
							
							if likeDensity == true then
							(
								CryArtistTools.rollouts[3].updateLBL.text = "Objects have same density"
								--print "Selected Objects have like density"
							)
							else
							(
								CryArtistTools.rollouts[3].updateLBL.text = "Multiple objects: different density"
								--print "Multiple objects: data mismatch"
								CryArtistTools.rollouts[3].useDensity.checked = false
								CryArtistTools.rollouts[3].objDensity.enabled = false
								CryArtistTools.rollouts[3].objDensity.value = 0
								CryArtistTools.rollouts[3].objScale.enabled = true
							)
						)
						else
						(
							CryArtistTools.rollouts[3].updateLBL.text = "Multiple objects: Data mismatch"
							CryArtistTools.rollouts[3].useDensity.checked = false
							CryArtistTools.rollouts[3].objDensity.enabled = false
							CryArtistTools.rollouts[3].objScale.enabled = false
							CryArtistTools.rollouts[3].objDensity.value = 0
						)
					)
				)
				"limit":
				(
					CryArtistTools.rollouts[3].jointLimit.enabled = true
					CryArtistTools.rollouts[3].jLimit.enabled = true
					CryArtistTools.rollouts[3].jTwist.enabled = true
					CryArtistTools.rollouts[3].jBend.enabled = true
					CryArtistTools.rollouts[3].jPull.enabled = true
					CryArtistTools.rollouts[3].jPush.enabled = true
					CryArtistTools.rollouts[3].jShift.enabled = true
					CryArtistTools.rollouts[3].jScale.enabled = true
					CryArtistTools.rollouts[3].jointScale.enabled = true
					CryArtistTools.rollouts[3].usejoint.checked = true
					CryArtistTools.rollouts[3].jointLimit.value = (testFor[2] as float)
					jointsPresent = true
					if multipleObj == true then
					(
						if AllLimit == true then
						(
							if likeLimit == true then
							(
								CryArtistTools.rollouts[3].jointLimit.value = (testFor[2] as float)
							)
							else
							(
								CryArtistTools.rollouts[3].jointLimit.value = 0
							)
						)
						else
						(
							CryArtistTools.rollouts[3].jointLimit.value = 0
						)
					)
				)
				"twist":
				(
					CryArtistTools.rollouts[3].jointTwist.enabled = true
					CryArtistTools.rollouts[3].jLimit.enabled = true
					CryArtistTools.rollouts[3].jTwist.enabled = true
					CryArtistTools.rollouts[3].jBend.enabled = true
					CryArtistTools.rollouts[3].jPull.enabled = true
					CryArtistTools.rollouts[3].jPush.enabled = true
					CryArtistTools.rollouts[3].jShift.enabled = true
					CryArtistTools.rollouts[3].jScale.enabled = true
					CryArtistTools.rollouts[3].jointScale.enabled = true
					CryArtistTools.rollouts[3].usejoint.checked = true
					CryArtistTools.rollouts[3].jointTwist.value = (testFor[2] as float)
					jointsPresent = true
					if multipleObj == true then
					(
						if AllTwist == true then
						(
							if likeTwist == true then
							(
								CryArtistTools.rollouts[3].jointTwist.value = (testFor[2] as float)
							)
							else
							(
								CryArtistTools.rollouts[3].jointTwist.value = 0
							)
						)
						else
						(
							CryArtistTools.rollouts[3].jointTwist.value = 0
						)
					)
				)
				"bend":
				(
					CryArtistTools.rollouts[3].jointBend.enabled = true
					CryArtistTools.rollouts[3].jLimit.enabled = true
					CryArtistTools.rollouts[3].jTwist.enabled = true
					CryArtistTools.rollouts[3].jBend.enabled = true
					CryArtistTools.rollouts[3].jPull.enabled = true
					CryArtistTools.rollouts[3].jPush.enabled = true
					CryArtistTools.rollouts[3].jShift.enabled = true
					CryArtistTools.rollouts[3].jScale.enabled = true
					CryArtistTools.rollouts[3].jointScale.enabled = true
					CryArtistTools.rollouts[3].usejoint.checked = true
					CryArtistTools.rollouts[3].jointBend.value = (testFor[2] as float)
					jointsPresent = true
					if multipleObj == true then
					(
						if AllBend == true then
						(
							if likeBend == true then
							(
								CryArtistTools.rollouts[3].jointBend.value = (testFor[2] as float)
							)
							else
							(
								CryArtistTools.rollouts[3].jointBend.value = 0
							)
						)
						else
						(
							CryArtistTools.rollouts[3].jointBend.value = 0
						)
					)
				)
				"pull":
				(
					CryArtistTools.rollouts[3].jointPull.enabled = true
					CryArtistTools.rollouts[3].jLimit.enabled = true
					CryArtistTools.rollouts[3].jTwist.enabled = true
					CryArtistTools.rollouts[3].jBend.enabled = true
					CryArtistTools.rollouts[3].jPull.enabled = true
					CryArtistTools.rollouts[3].jPush.enabled = true
					CryArtistTools.rollouts[3].jShift.enabled = true
					CryArtistTools.rollouts[3].jScale.enabled = true
					CryArtistTools.rollouts[3].jointScale.enabled = true
					CryArtistTools.rollouts[3].usejoint.checked = true
					CryArtistTools.rollouts[3].jointPull.value = (testFor[2] as float)
					jointsPresent = true
					if multipleObj == true then
					(
						if AllPull == true then
						(
							if likePull == true then
							(
								CryArtistTools.rollouts[3].jointPull.value = (testFor[2] as float)
							)
							else
							(
								CryArtistTools.rollouts[3].jointPull.value = 0
							)
						)
						else
						(
							CryArtistTools.rollouts[3].jointPull.value = 0
						)
					)
				)
				"push":
				(
					CryArtistTools.rollouts[3].jointPush.enabled = true
					CryArtistTools.rollouts[3].jLimit.enabled = true
					CryArtistTools.rollouts[3].jTwist.enabled = true
					CryArtistTools.rollouts[3].jBend.enabled = true
					CryArtistTools.rollouts[3].jPull.enabled = true
					CryArtistTools.rollouts[3].jPush.enabled = true
					CryArtistTools.rollouts[3].jShift.enabled = true
					CryArtistTools.rollouts[3].jScale.enabled = true
					CryArtistTools.rollouts[3].jointScale.enabled = true
					CryArtistTools.rollouts[3].usejoint.checked = true
					CryArtistTools.rollouts[3].jointPush.value = (testFor[2] as float)
					jointsPresent = true
					if multipleObj == true then
					(
						if AllPush == true then
						(
							if likePush == true then
							(
								CryArtistTools.rollouts[3].jointPush.value = (testFor[2] as float)
							)
							else
							(
								CryArtistTools.rollouts[3].jointPush.value = 0
							)
						)
						else
						(
							CryArtistTools.rollouts[3].jointPush.value = 0
						)
					)
				)
				"shift":
				(
					CryArtistTools.rollouts[3].jointShift.enabled = true
					CryArtistTools.rollouts[3].jLimit.enabled = true
					CryArtistTools.rollouts[3].jTwist.enabled = true
					CryArtistTools.rollouts[3].jBend.enabled = true
					CryArtistTools.rollouts[3].jPull.enabled = true
					CryArtistTools.rollouts[3].jPush.enabled = true
					CryArtistTools.rollouts[3].jShift.enabled = true
					CryArtistTools.rollouts[3].jScale.enabled = true
					CryArtistTools.rollouts[3].jointScale.enabled = true
					CryArtistTools.rollouts[3].usejoint.checked = true
					CryArtistTools.rollouts[3].jointShift.value = (testFor[2] as float)
					jointsPresent = true
					if multipleObj == true then
					(
						if AllShift == true then
						(
							if likeShift == true then
							(
								CryArtistTools.rollouts[3].jointShift.value = (testFor[2] as float)
							)
							else
							(
								CryArtistTools.rollouts[3].jointShift.value = 0
							)
						)
						else
						(
							CryArtistTools.rollouts[3].jointShift.value = 0
						)
					)
				)
				"pickable":
				(
					CryArtistTools.rollouts[3].ispickable.checked = true
				)
				"entity":
				(
					CryArtistTools.rollouts[3].isEntity.checked = true
				)
				"rotaxes":
				(
					if testFor[2] == "x" then CryArtistTools.rollouts[3].xrot.checked = true
					if testFor[2] == "y" then CryArtistTools.rollouts[3].yrot.checked = true
					if testFor[2] == "z" then CryArtistTools.rollouts[3].zrot.checked = true
					if testFor[2] == "xy"then
					(
						CryArtistTools.rollouts[3].xrot.checked = true
						CryArtistTools.rollouts[3].yrot.checked = true
					)
					if testFor[2] == "xz"then
					(
						CryArtistTools.rollouts[3].xrot.checked = true
						CryArtistTools.rollouts[3].zrot.checked = true
					)
					if testFor[2] == "yz"then
					(
						CryArtistTools.rollouts[3].zrot.checked = true
						CryArtistTools.rollouts[3].yrot.checked = true
					)
					if testFor[2] == "xyz"then
					(
						CryArtistTools.rollouts[3].xrot.checked = true
						CryArtistTools.rollouts[3].Yrot.checked = true
						CryArtistTools.rollouts[3].zrot.checked = true
					)
				)
				"sizevar":
				(
					CryArtistTools.rollouts[3].sizevar.value = (testFor[2] as float)
				)
				"generic":
				(
					CryArtistTools.rollouts[3].GenericCount.value = (testFor[2] as float)
				)
			)
			case udpInARR[i] of
			(
				"cylinder": CryArtistTools.rollouts[3].cylinder1.checked = true
				"box": CryArtistTools.rollouts[3].box1.checked = true
				"capsule": CryArtistTools.rollouts[3].capsule1.checked = true
				"sphere": CryArtistTools.rollouts[3].sphere1.checked = true
			)
			if jointsPresent == true then
			(
				CryArtistTools.rollouts[3].jointLimit.enabled = true
				CryArtistTools.rollouts[3].jointTwist.enabled = true
				CryArtistTools.rollouts[3].jointBend.enabled = true
				CryArtistTools.rollouts[3].jointPull.enabled = true
				CryArtistTools.rollouts[3].jointPush.enabled = true
				CryArtistTools.rollouts[3].jointShift.enabled = true
			)
		)
	)
)
-------------------------------End of Update Function---------------------------------------------

-------------------------------------------------------------------------------
--	CryArtistTools interface
-------------------------------------------------------------------------------

(
if CryArtistTools != undefined do ( closerolloutfloater CryArtistTools )
CryArtistTools = newrolloutfloater "CryArtistTools 1.4" 190 380
	
--ArtistTools
-------------------------------------------------------------------------------
rollout ArtistTools "Artist Tools"
(
	group "General Tools"
	(
		button centerPivotBttn "centerPivot" offset:[-42,0]
		checkbutton preserveUV "preserveUVs" offset:[40,-26]
		button resetXformBttn "resetXformCollapse"
		checkbutton copyModifier "Copy Modifier" height:18 width:120
		button applyModifier "Select a modifier" height:18 width:120 enabled:false
	)
	
	button exportCheck "Export Check"
	
	Group "UV Tools"
	(
		button scaleVert images:#((crytools.BuildPathFull + "tools\\maxscript\\icons\\scalevert.png"), undefined, 1, 1, 1, 1, 1) tooltip:"collapse vertical" offset:[-65,0]
		button scaleHoriz images:#((crytools.BuildPathFull + "tools\\maxscript\\icons\\scaleHoriz.png"), undefined, 1, 1, 1, 1, 1) tooltip:"collapse horizontal" offset:[-35,-31]
		button scaleUniform images:#((crytools.BuildPathFull + "tools\\maxscript\\icons\\scaleUniform.png"), undefined, 1, 1, 1, 1, 1) tooltip:"uniform scale" offset:[-5,-31]
		button rot90c images:#((crytools.BuildPathFull + "tools\\maxscript\\icons\\90c.png"), undefined, 1, 1, 1, 1, 1) tooltip:"rotate 90 clockwise" offset:[25,-31]
		button rot90cc images:#((crytools.BuildPathFull + "tools\\maxscript\\icons\\90cc.png"), undefined, 1, 1, 1, 1, 1) tooltip:"rotate 90 counter clockwise" offset:[55,-31]
		label OBJvertsLBL "OBJ: " align:#left offset:[0,5]
		label UVvertsLBL "UV: " align:#left offset:[70,-18.5]
		button refreshVerts "<" align:#right offset:[0,-23]
	)
	
	Group "Modelling Tools"
	(
		button LimEloop images:#((crytools.BuildPathFull + "tools\\maxscript\\icons\\LEloop.png"), undefined, 1, 1, 1, 1, 1) tooltip:"Limited Edge Loop" offset:[-65,0]
		button LimEring images:#((crytools.BuildPathFull + "tools\\maxscript\\icons\\LEring.png"), undefined, 1, 1, 1, 1, 1) tooltip:"Limited Edge Ring" offset:[-35,-31]
		button LimVloop images:#((crytools.BuildPathFull + "tools\\maxscript\\icons\\LVloop.png"), undefined, 1, 1, 1, 1, 1) tooltip:"Limited Vert Loop" offset:[-5,-31]
		button LimVring images:#((crytools.BuildPathFull + "tools\\maxscript\\icons\\LVring.png"), undefined, 1, 1, 1, 1, 1) tooltip:"Limited Vert Ring" offset:[25,-31]
		button LimPloop images:#((crytools.BuildPathFull + "tools\\maxscript\\icons\\LPloop.png"), undefined, 1, 1, 1, 1, 1) tooltip:"Limited Poly Ring" offset:[55,-31]
	)
	
	--	centerPivot
	on centerPivotBttn pressed do
	(
		for obj in selection do
		(
			CenterPivot obj
		)
	)
	
	--	refreshVerts 
	on refreshVerts  pressed do
	(
		if selection.count != 1 then
		(
			OBJvertsLBL.text = "OBJ: --"
			UVvertsLBL.text = "UV: --"
		)
		else
		(
			try
			(
				OBJvertsLBL.text = ("OBJ: " + ($.numverts as string))
				UVvertsLBL.text = ("UV: " + ($.modifiers[#unwrap_uvw].unwrap.numbervertices() as string))
			)
			catch()
		)
	)

	--	preserveUVs
	on preserveUV changed state do
	(
	try
		if (preserveUV.checked == true) then 
		(
			$.preserveUVs = on
		)
		else
		(
			$.preserveUVs = off
		)
		catch
		( 
			messageBox "Please select an object. Or make sure that your object has an edit poly applied." title:"Error"
			preserveUV.checked = false
			return undefined
		)
	)
	
	on ArtistTools open do
	(
		try
		(
			FileIn (BuildPathFull_crytools + "Tools\\maxscript\\CryModelling.ms")
			if $.modifiers[#unwrap_uvw] != undefined then
			(
				OBJvertsLBL.text = ("OBJ: " + ($.numverts as string))
				UVvertsLBL.text = ("UV: " + ($.modifiers[#unwrap_uvw].unwrap.numbervertices() as string))
			)
		)
		catch()
	)
	
	--	resetXformCollapse
	on resetXformBttn pressed do
	(
		for obj in selection do
		(
			ResetXForm obj
			print (obj.name + " xforms reset")
			modPanel.setCurrentObject obj.baseObject
			maxOps.CollapseNode obj off
		)
	)
	
	--	uv tools
	on scaleVert pressed do
	(
		try($.modifiers[#unwrap_uvw].unwrap2.ScaleSelectedCenter 0.001 2)catch()
	)
	
	on scaleHoriz pressed do
	(
		try($.modifiers[#unwrap_uvw].unwrap2.ScaleSelectedCenter 0.001 1)catch()
	)
	
	on scaleUniform pressed do
	(
		try($.modifiers[#unwrap_uvw].unwrap2.ScaleSelectedCenter 0.001 0)catch()
	)
	
	on rot90c pressed do
	(
		try($.modifiers[#unwrap_uvw].unwrap2.RotateSelectedCenter -1.59698)catch()
	)
	
	on rot90cc pressed do
	(
		try($.modifiers[#unwrap_uvw].unwrap2.RotateSelectedCenter 1.6057)catch()
	)
	
	--exportCheck
	on exportCheck pressed do
	(
		if $ == undefined then
		(
			messageBox "No object selected" title:"Error"
			return undefined
		)
		if $.material == undefined then
		(
			messageBox "There is no defined shader for this object!" title:"Problem Found!"
			return undefined
		)
		materials = $.material.materialIDlist
		for i = 1 to materials.count do
		(
			checkme = $.material.material[i].shadername
			if checkme != "Crytek Shader" then
			(
				messageBox ("The following material is not set to the Crytek Shader: " + $.material.material[i].name) title:"Problem Found!"
			)
		)
	)
	
	on applyModifier pressed do
	(
		if xferModifier == undefined then return 0
		else
		(
			undo "paste modifiers" on
			(
				for obj in selection do
				(
					if validModifier obj xferModifier then addmodifier obj (copy xferModifier)
					print "test"
				)
			)
		)
	)

	-- modified from Velmar cgsociety post
	on copyModifier changed state do
	(
		local o,s
		if copyModifier.checked = true then
		(
			if selection.count == 1 then
			(
				try
				(
					o = selection[1]
					s = modPanel.getcurrentObject()
					if s != undefined then
					(
						s = (modPanel.getModifierIndex o s)
						print s
						global xferModifier = copy o.modifiers[s]
						copyModifier.caption = o.modifiers[s].name
						applyModifier.enabled = true
						applyModifier.caption = "Apply Modifier"
						print ("copying " + (xferModifier as string))
					)
					else copyModifier.checked = off
				)
			catch
				(
					copyModifier.checked = off
					print "error"
				)
			)
			else copyModifier.checked = off
		)
		else
		(
			copyModifier.caption = "Copy Modifier"
			xferModifier = undefined
			applyModifier.caption = "Select a modifier"
			applyModifier.enabled = false
		)
	)
	
	---------Modelling Tools------------------
	on LimEloop pressed do
	(
		CryModelling.SelLimELoop ()
	)
	
	on LimEring pressed do
	(
		CryModelling.SelLimERing()
	)
	
	on LimVloop pressed do
	(
		CryModelling.SelVertLoop()
	)
	
	on LimVring pressed do
	(
		CryModelling.SelVertRing()
	)
	
	on LimPloop pressed do
	(
		CryModelling.SelPolyLoop()
	)
)


rollout Diagnostics "Diagnostics"
(
	fn updatePolyStats = 
	(
		if CryArtistTools.rollouts[2].open == true then
		(
			stats = crytools.polyStats()
			CryArtistTools.rollouts[2].faceLBL.text = ("Faces: " + stats[1] as string)
			CryArtistTools.rollouts[2].edgeLBL.text = ("Edges: " + stats[2] as string)
			CryArtistTools.rollouts[2].vertLBL.text = ("Verts: " + stats[3] as string)
			print (("Faces: " + stats[1] as string) + (" Edges: " + stats[2] as string) + (" Verts: " + stats[3] as string))
		)
	)
	
	fn thinFatStats =
	(
		if CryArtistTools.rollouts[2].open == true then
		(
			if selection.count != 1 then
			(
				messagebox "select one mesh"
				return undefined
			)
			base = (crytools.forcelowercase $.name)
			base = crytools.cutstring base "_LOD1"
			base = crytools.cutstring base "_LOD2"
			base = crytools.cutstring base "_LOD3"
			base = crytools.cutstring base "_thin"
			base = crytools.cutstring base "_fat"
			
			outText = ""
			try(outText = (((getnodebyname base).mesh.verts.count as string) + "   " + base + "\n"))catch(outText = "BASE_LOD0: NOT FOUND\n")
			try(outText += (((getnodebyname (base + "_thin")).mesh.verts.count as string) + "   " + base + "_thin\n"))catch(outText += "THIN_LOD0: NOT FOUND\n")
			try(outText += (((getnodebyname (base + "_fat")).mesh.verts.count as string) + "   " + base + "_fat\n"))catch(outText += "FAT_LOD0: NOT FOUND\n")
			try(outText += (((getnodebyname (base + "_LOD1")).mesh.verts.count as string) + "   " + base + "_LOD1\n"))catch(outText += "LOD1: NOT FOUND\n")
			try(outText += (((getnodebyname (base + "_thin_LOD1")).mesh.verts.count as string) + "   " + base + "_thin_LOD1\n"))catch(outText += "THIN_LOD1: NOT FOUND\n")
			try(outText += (((getnodebyname (base + "_fat_LOD1")).mesh.verts.count as string) + "   " + base + "_fat_LOD1\n"))catch(outText += "FAT_LOD1: NOT FOUND\n")
			try(outText += (((getnodebyname (base + "_LOD2")).mesh.verts.count as string) + "   " + base + "_LOD2\n"))catch(outText += "LOD2: NOT FOUND\n")
			try(outText += (((getnodebyname (base + "_thin_LOD2")).mesh.verts.count as string) + "   " + base + "_thin_LOD2\n"))catch(outText += "THIN_LOD2: NOT FOUND\n")
			try(outText += (((getnodebyname (base + "_fat_LOD2")).mesh.verts.count as string) + "   " + base + "_fat_LOD2\n"))catch(outText += "FAT_LOD2: NOT FOUND\n")				
			try(outText += (((getnodebyname (base + "_LOD3")).mesh.verts.count as string) + "   " + base + "_LOD3\n"))catch(outText += "LOD3: NOT FOUND\n")
			try(outText += (((getnodebyname (base + "_thin_LOD3")).mesh.verts.count as string) + "   " + base + "_thin_LOD3\n"))catch(outText += "THIN_LOD3: NOT FOUND\n")			
			try(outText += (((getnodebyname (base + "_fat_LOD3")).mesh.verts.count as string) + "   " + base + "_fat_LOD2"))catch(outText += "FAT_LOD3: NOT FOUND")
			
			messagebox outText title:"Thin/Fat Mesh Check"
			if cryartisttools.rollouts[2].printThinFat.checked == true then (print outText)
		)
	)
	
	group "polyStats"
	(
		label faceLBL "Faces: " align:#left
		label edgeLBL "Edges: " align:#left
		label vertLBL "Verts: " align:#left
	)
	group "thin/fat meshCheck"
	(
		button thinFat "Thin/Fat Mesh Check"
		checkbox printThinFat "Print to Listener" align:#center checked:true
	)
	
	on Diagnostics rolledUp state do
	(
		if state == true then
		(
			CryArtistTools.rollouts[2].updatePolyStats()
			callbacks.addScript #selectionSetChanged "CryArtistTools.rollouts[2].updatePolyStats()" id:#updatePolyStats
		)
		else
		(
			callbacks.removeScripts id:#updatePolyStats
		)
	)
	
	/*on Diagnostics open do
	(
		callbacks.addScript #selectionSetChanged "CryArtistTools.rollouts[2].updatePolyStats()" id:#updatePolyStats
	)*/
	
	on Diagnostics close do
	(
		callbacks.removeScripts id:#updatePolyStats
	)
	
	on thinFat pressed do
	(
		thinFatStats()
	)
)

rollout metadataManager "CGF Metadata Manager"
(
	label updateLBL "Info will be updated here"
	button UpdateUDP " Update Data " Height:25 Enabled:false
	button udpSetToSel "     Apply Settings to Selected     " height:25
	checkbox useMass "Mass:" offset:[-10,0]
	spinner objMass "" type:#float range:[0,99999,0] fieldWidth:50 offset:[-40,-20]
	checkbox useDensity "Density:" offset:[-10,0]
	spinner objDensity "" type:#float range:[0,99999,0] fieldWidth:50 offset:[-40,-20]
	spinner objScale "Scale" type:#float range:[0,99999,100] fieldWidth:50 offset:[-40,0]		
	button selectAllMatching "Select Objects Matching Details"
	dropdownlist SelObjs items:#("mass", "density", "limit", "twist", "bend", "pull", "push", "shift") width:65 offset:[-10,0]
	dropdownlist GrtrOrSmlr items:#( "=", ">", "<") width:40 offset:[63,-27.5]
	spinner SelVal type:#float range:[0,99999,0] fieldWidth:40 offset:[7,-25]

	group "Force Primitives"
	(
		checkbox box1 "box"
		checkbox cylinder1 "cylinder" offset:[40,-20]
		checkbox sphere1 "sphere" offset:[100,-20]
		checkbox capsule1 "capsule"
	)
	
	group "Joints"
	(
		checkbox useJoint "Enable joint properties"
		checkbox jLimit "Limit:" offset:[55,0]
		checkbox jTwist "Twist:" offset:[55,0]
		checkbox jBend "Bend:" offset:[55,0]
		checkbox jPull "Pull:" offset:[55,0]
		checkbox jPush "Push:" offset:[55,0]
		checkbox jShift "Shift:" offset:[55,0]
		spinner jointLimit type:#float fieldWidth:35 offset:[0,-120]
		spinner jointTwist type:#float fieldWidth:35 offset:[0,0]
		spinner jointBend type:#float fieldWidth:35 offset:[0,0]
		spinner jointPull type:#float fieldWidth:35 offset:[0,0]
		spinner jointPush type:#float fieldWidth:35 offset:[0,0]
		spinner jointShift type:#float fieldWidth:35 offset:[0,0]
		checkbox jScale "Scale Properties:" offset:[0,10]
		spinner jointScale type:#float range:[0,99999,100] fieldWidth:40 offset:[0,-20]
	)
	--group "General"
	--(
		--checkbox isPickable "pickable"
	--)
	group "destroyableObjects"
	(
		checkbox isMain "main"
		checkbox isRemain "remain" offset:[47,-20]
		spinner GenericCount "Generic:" type:#float fieldWidth:35 offset:[-70,0]
		label spawnLocTXT "__Spawn Location_____________"
		checkbox isEntity "entity"
		--checkbox isBone "bone"
		--edittext isBoneTXT "" offset:[45,-21] fieldWidth:60
		--pickbutton pickBone "pick" offset:[62,-24]
		label rotAxesTXT "rot axes" offset:[-60,0]
		checkbutton xrot "x" offset:[-25,-21]
		checkbutton yrot "y" offset:[0,-26]
		checkbutton zrot "z" offset:[25,-26]
		spinner sizeVar "size var:" type:#float fieldWidth:35 offset:[-70,0]
	)
	button openMetaDataLister "Open CGF Metadata Lister" enabled:false
	
	on selectAllMatching pressed do
	(
		clearSelection()
		for obj in objects do
		(
			UDPdata = crytools.inFromUDP obj
			for x = 1 to UDPdata.count do
			(
				if (filterstring UDPdata[x] " =")[1] == SelObjs.selected then
				(
					EquateVal = GrtrOrSmlr.selected
					case EquateVal of
					(
						"=":
						(
							if (filterstring UDPdata[x] " =")[2] as float == SelVal.value then selectmore obj
						)
						
						">":
						(
							if (filterstring UDPdata[x] " =")[2] as float > SelVal.value then selectmore obj
						)
						
						"<":
						(
							if (filterstring UDPdata[x] " =")[2] as float < SelVal.value then selectmore obj
						)
					)
				)
			)
		)
	)
	
	on metadataManager open do
	(
		--isBoneTXT.enabled = false
		--isBoneTXT.text = "bone name"
		objDensity.value = 0
		objMass.value = 0
		objDensity.enabled = false
		objMass.enabled = false
		udpGetFromSel()
		
		--callbacks.removeScripts id:#updateUDP
		--callbacks.addScript #selectionSetChanged "udpGetFromSel()" id:#updateUDP
	)
	
	
	on metadataManager rolledUp state do
	(
		if state then --set callback for dynamic change based on selection
		(
			callbacks.removeScripts id:#updateUDP
			callbacks.addScript #selectionSetChanged "udpGetFromSel()" id:#updateUDP
		)
		else --removie callbacks if the rollout is rolled up
		(
			callbacks.removeScripts id:#updateUDP
		)
	)
	
	on useMass changed state do --Mass checkbox behaviour
	(
		if useMass.checked == true then
		(
			if useJoint.checked == true then
			(
				messagebox "Joints cannot have a mass or density!"
				useMass.checked = false
				return undefined
			)
			objMass.enabled = true
			objDensity.enabled = false
			useDensity.checked = false
		)
		else
		(
			objMass.enabled = false
		)
	)
	
	on useDensity changed state do --Density checkbox behaviour
	(
		if useDensity.checked == true then
		(
			if useJoint.checked == true then
			(
				messagebox "Joints cannot have a mass or density!"
				useDensity.checked = false
				return undefined
			)
			objDensity.enabled = true
			objMass.enabled = false
			useMass.checked = false
		)
		else
		(
			objDensity.enabled = false
		)
	)
	
	--Primitives checkboxes behaviors
	on box1 changed state do
	(
		if box1.checked == true then
		(
			cylinder1.checked = false
			sphere1.checked = false
			capsule1.checked = false
		)
	)
	
	on cylinder1 changed state do
	(
		if cylinder1.checked == true then
		(
			box1.checked = false
			sphere1.checked = false
			capsule1.checked = false
		)
	)
	
	on sphere1 changed state do
	(
		if sphere1.checked == true then
		(
			cylinder1.checked = false
			box1.checked = false
			capsule1.checked = false
		)
	)
	
	on capsule1 changed state do
	(
		if capsule1.checked == true then
		(
			cylinder1.checked = false
			sphere1.checked = false
			box1.checked = false
		)
	)
	
	--Joint Properties checkboxes behaviors
	on jLimit changed state do
	(
		if jLimit.checked == true then
		(
			jointLimit.enabled = false
		)
		else
		(
			jointLimit.enabled = true
		)
	)
	
	on jTwist changed state do
	(
		if jTwist.checked == true then
		(
			jointTwist.enabled = false
		)
		else
		(
			jointTwist.enabled = true
		)
	)
	
	on jBend changed state do
	(
		if jBend.checked == true then
		(
			jointBend.enabled = false
		)
		else
		(
			jointBend.enabled = true
		)
	)
	
	on jPull changed state do
	(
		if jPull.checked == true then
		(
			jointPull.enabled = false
		)
		else
		(
			jointPull.enabled = true
		)
	)

	on jPush changed state do
	(
		if jPush.checked == true then
		(
			jointPush.enabled = false
		)
		else
		(
			jointPush.enabled = true
		)
	)
	
	on jShift changed state do
	(
		if jShift.checked == true then
		(
			jointShift.enabled = false
		)
		else
		(
			jointShift.enabled = true
		)
	)
	on jScale changed state do
	(
		if jScale.checked == true then
		(
			jointScale.enabled = true
			
			jointLimit.enabled = false
			jointTwist.enabled = false
			jointBend.enabled = false
			jointPull.enabled = false
			jointPush.enabled = false
			jointShift.enabled = false
		)
		else
		(
			jointScale.enabled = false
			
			jointLimit.enabled = true
			jointTwist.enabled = true
			jointBend.enabled = true
			jointPull.enabled = true
			jointPush.enabled = true
			jointShift.enabled = true
		)
	)
	
---------------------------------------------	
--------------Applying Settings------------
---------------------------------------------		
	
	on udpSetToSel pressed do
	(
		for obj in selection do
		(
			udpInARR = crytools.inFromUDP obj
			
			-- MASS OUTPUT ----------------------------------------
			if useMass.checked == true then
			(
				massVal = ("mass = " + objMass.value as string)
				crytools.LookAndWrite obj massVal
			)
			else
			(
				cryTools.LookAndDelete obj "mass"
			)
			
			-- DENSITY OUTPUT ----------------------------------------
			if useDensity.checked == true then
			(
				densityVal = ("density = " + objDensity.value as string)
				crytools.LookAndWrite obj densityVal
			)
			else
			(
				cryTools.LookAndDelete obj "density"
			)
			
			-- FORCE PRIMITIVES OUTPUT ----------------------------------------
			if box1.checked == true then
			(
				for obj in selection do
				(
					crytools.LookAndWrite obj "box"
				)
			)
			else
			(
				for obj in selection do
				(
					cryTools.LookAndDelete obj "box"
				)
			)
			if sphere1.checked == true then
			(
				for obj in selection do
				(
					crytools.LookAndWrite obj "sphere"
				)
			)
			else
			(
				for obj in selection do
				(
					cryTools.LookAndDelete obj "sphere"
				)
			)
			if cylinder1.checked == true then
			(
				for obj in selection do
				(
					crytools.LookAndWrite obj "cylinder"
				)
			)
			else
			(
				for obj in selection do
				(
					cryTools.LookAndDelete obj "cylinder"
				)
			)
			if capsule1.checked == true then
			(
				for obj in selection do
				(
					crytools.LookAndWrite obj "capsule"
				)
			)
			else
			(
				for obj in selection do
				(
					cryTools.LookAndDelete obj "capsule"
				)
			)
			
			--------JOINT PROPERTIES---------
			if useJoint.checked == true then
			(
				if jointLimit.value != 0 then
				(
					jLim = ("limit = " + jointLimit.value as string)
					crytools.LookAndWrite obj jLim
				)
				else
				(
					if jScale.checked == false then
					(
						cryTools.LookAndDelete obj "limit"
					)
				)
				
				if jointTwist.enabled == true and jointTwist.value != 0 then
				(
					jTwis = ("twist = " + jointTwist.value as string)
					crytools.LookAndWrite obj jTwis
				)
				else
				(
					if jScale.checked == false then
					(
						cryTools.LookAndDelete obj "twist"
					)
				)
				
				if jointBend.value != 0 then
				(
					jBen = ("bend = " + jointBend.value as string)
					crytools.LookAndWrite obj jBen
				)
				else
				(
					if jScale.checked == false then
					(
						cryTools.LookAndDelete obj "bend"
					)
				)
				
				if jointPull.value != 0 then
				(
					jPul = ("pull = " + jointPull.value as string)
					crytools.LookAndWrite obj jPul
				)
				else
				(
					if jScale.checked == false then
					(
						cryTools.LookAndDelete obj "pull"
					)
				)
				
				if jointPush.value != 0 then
				(
					jPus = ("push = " + jointPush.value as string)
					crytools.LookAndWrite obj jPus
				)
				else
				(
					if jScale.checked == false then
					(
						cryTools.LookAndDelete obj "push"
					)
				)
				
				if jointShift.value != 0 then
				(
					jShif = ("shift = " + jointShift.value as string)
					crytools.LookAndWrite obj jShif
				)
				else
				(
					if jScale.checked == false then
					(
						cryTools.LookAndDelete obj "shift"
					)
				)
				
				---------Scale Joint Properties----------
				jSclVal = (jointScale.value/100)
				if jSclVal != 1 then
				(
					if jLimit.checked == true then
					(
						if jointLimit.value != 0 then
						(
							jLim = ("limit = " + (jointLimit.value * jSclVal) as string)
							crytools.LookAndWrite obj jLim
						)
						else
						(
							if (crytools.getUDP obj "limit") !=0 then
							(
								jLim = ("limit = " + ((crytools.getUDP obj "limit") * jSclVal) as string)
								crytools.LookAndWrite obj jLim
							)
						)
					)
					
					if jTwist.checked == true then
					(
						if jointTwist.value != 0 then
						(
							jTwis = ("twist = " + (jointTwist.value*jSclVal) as string)
							crytools.LookAndWrite obj jTwis
						)
						else
						(
							if (crytools.getUDP obj "twist") !=0 then
							(
								jTwis = ("twist = " + ((crytools.getUDP obj "twist") * jSclVal) as string)
								crytools.LookAndWrite obj jLim
							)
						)
					)
					
					if jBend.checked == true then
					(
						if jointBend.value != 0 then
						(
							jBen = ("bend = " + (jointBend.value*jSclVal) as string)
							crytools.LookAndWrite obj jBen
						)
						else
						(
							if (crytools.getUDP obj "bend") !=0 then
							(
								jBen = ("bend = " + ((crytools.getUDP obj "bend") * jSclVal) as string)
								crytools.LookAndWrite obj jBen
							)
						)
					)
					
					if jPull.checked == true then
					(
						if jointPull.value != 0 then
						(
							jPul = ("pull = " + (jointPull.value*jSclVal) as string)
							crytools.LookAndWrite obj jPul
						)
						else
						(
							if (crytools.getUDP obj "pull") !=0 then
							(
								jPul = ("pull = " + ((crytools.getUDP obj "pull") * jSclVal) as string)
								crytools.LookAndWrite obj jPul
							)
						)
					)
					
					if jPush.checked == true then
					(
						if jointPush.value != 0 then
						(
							jPus = ("push = " + (jointPush.value*jSclVal) as string)
							crytools.LookAndWrite obj jPus
						)
						else
						(
							if (crytools.getUDP obj "push") !=0 then
							(
								jPus = ("push = " + ((crytools.getUDP obj "push") * jSclVal) as string)
								crytools.LookAndWrite obj jPus
							)
						)
					)
					
					if jShift.checked == true then
					(
						if jointShift.value != 0 then
						(
							jShif = ("shift = " + (jointShift.value*jSclVal) as string)
							crytools.LookAndWrite obj jShif
						)
						else
						(
							if (crytools.getUDP obj "shift") !=0 then
							(
								jShif = ("shift = " + ((crytools.getUDP obj "shift") * jSclVal) as string)
								crytools.LookAndWrite obj jShif
							)
						)
					)
				)
			)
			else
			(
				cryTools.LookAndDelete obj "limit"
				cryTools.LookAndDelete obj "twist"
				cryTools.LookAndDelete obj "bend"
				cryTools.LookAndDelete obj "pull"
				cryTools.LookAndDelete obj "push"
				cryTools.LookAndDelete obj "shift"
			)
			
			------------Destroyable Objects------------
			if isMain.checked == true then obj.name = "Main"
			if isRemain.checked == true then obj.name = "Remain"
			
			if isEntity.checked == true then
			(
				crytools.LookAndWrite obj "entity"
			)
			else
			(
				crytools.LookAndDelete obj "entity"
			)
			
			if xrot.checked == true then
			(
				AxisRotations = "rotaxes = x"
				if yrot.checked == true then
				(
					AxisRotations = "rotaxes = xy"
					if zrot.checked == true then
					(
						AxisRotations = "rotaxes = xyz"
					)
				)
				else
				(
					if zrot.checked == true then
					(
						AxisRotations = "rotaxes = xz"
					)
				)
				crytools.LookAndWrite obj AxisRotations
			)
			
			if yrot.checked == true then
			(
				AxisRotations = "rotaxes = y"
				if xrot.checked == true then
				(
					AxisRotations = "rotaxes = xy"
					if zrot.checked == true then
					(
						AxisRotations = "rotaxes = xyz"
					)
				)
				else
				(
					if zrot.checked == true then
					(
						AxisRotations = "rotaxes = yz"
					)
				)
				crytools.LookAndWrite obj AxisRotations
			)
			
			if zrot.checked == true then
			(
				AxisRotations = "rotaxes = z"
				if yrot.checked == true then
				(
					AxisRotations = "rotaxes = yz"
					if xrot.checked == true then
					(
						AxisRotations = "rotaxes = xyz"
					)
				)
				else
				(
					if xrot.checked == true then
					(
						AxisRotations = "rotaxes = xz"
					)
				)
				crytools.LookAndWrite obj AxisRotations
			)
			if xrot.checked ==false and yrot.checked==false and zrot.checked==false then
			(
				crytools.LookAndDelete obj "rotaxes"
			)
			
			if sizeVar.value != 0 then
			(
				Var = ("sizevar = " + sizeVar.value as string)
				crytools.LookAndWrite obj Var
			)
			else
			(
				crytools.LookAndDelete obj "sizevar"
			)
			
			if  GenericCount.value != 0 then
			(
				GenCount = ("generic = " + GenericCount.value as string)
				crytools.LookAndWrite obj GenCount
			)
			else
			(
				crytools.LookAndDelete obj "generic"
			)
		)
	)
	
	------------------------------------------------	
	--------------Updating Settings--------------
	------------------------------------------------
	
	on UpdateUDP pressed do
	(
		SclVal =(objScale.value/100)
		for obj in selection do
		(
			udpInARR = crytools.inFromUDP obj
			-- SCALE OUTPUT ---------------------------------------
			if SclVal != 1 then
			(
				--Scale Mass--
				testFor = filterstring udpInARR[1] " ="
										
				if testFor[1] == "mass" then
				(
					massVal = ("mass = " + ((testFor[2] as float)*SclVal)as string)
					udpInARR[1] = massVal
					crytools.outToUDP udpInARR obj false
				)
				--Scale Density--
				if  testFor[1] == "density"  then
				(
					densityVal = ("density ="  + ((testFor[2] as float)*SclVal)as string)
					udpInARR[1] = densityVal
					crytools.outToUDP udpInARR obj false
				)
			)
			
			-- MASS OUTPUT ---------------------------------------
			
			if useMass.checked == true then
			(
				massVal = ("mass = " + objMass.value as string)
				crytools.LookAndWrite obj massVal
			)
			else
			(
			
			)
			
			-- DENSITY OUTPUT ----------------------------------------
			if useDensity.checked == true then
			(
				densityVal = ("density = " + objDensity.value as string)
				crytools.LookAndWrite obj densityVal
			)
			else
			(
				
			)
			
			-- FORCE PRIMITIVES OUTPUT ----------------------------------------
			if box1.checked == true then
			(
				for obj in selection do
				(
					crytools.LookAndWrite obj "box"
				)
			)
			else
			(
				for obj in selection do
				(
					cryTools.LookAndDelete obj "box"
				)
			)
			if sphere1.checked == true then
			(
				for obj in selection do
				(
					crytools.LookAndWrite obj "sphere"
				)
			)
			else
			(
				for obj in selection do
				(
					cryTools.LookAndDelete obj "sphere"
				)
			)
			if cylinder1.checked == true then
			(
				for obj in selection do
				(
					crytools.LookAndWrite obj "cylinder"
				)
			)
			else
			(
				for obj in selection do
				(
					cryTools.LookAndDelete obj "cylinder"
				)
			)
			if capsule1.checked == true then
			(
				for obj in selection do
				(
					crytools.LookAndWrite obj "capsule"
				)
			)
			else
			(
				for obj in selection do
				(
					cryTools.LookAndDelete obj "capsule"
				)
			)
			
			--------JOINT PROPERTIES---------
			if useJoint.checked == true then
			(
				if jointLimit.value != 0 then
				(
					jLim = ("limit = " + jointLimit.value as string)
					crytools.LookAndWrite obj jLim
				)
				else
				(
					cryTools.LookAndDelete obj "limit"
				)
				
				if jointTwist.value != 0 then
				(
					jTwis = ("twist = " + jointTwist.value as string)
					crytools.LookAndWrite obj jTwis
				)
				else
				(
					cryTools.LookAndDelete obj "twist"
				)
				
				if jointBend.value != 0 then
				(
					jBen = ("bend = " + jointBend.value as string)
					crytools.LookAndWrite obj jBen
				)
				else
				(
					cryTools.LookAndDelete obj "bend"
				)
				
				if jointPull.value != 0 then
				(
					jPul = ("pull = " + jointPull.value as string)
					crytools.LookAndWrite obj jPul
				)
				else
				(
					cryTools.LookAndDelete obj "pull"
				)
				
				if jointPush.value != 0 then
				(
					jPus = ("push = " + jointPush.value as string)
					crytools.LookAndWrite obj jPus
				)
				else
				(
					cryTools.LookAndDelete obj "push"
				)
				
				if jointShift.value != 0 then
				(
					jShif = ("shift = " + jointShift.value as string)
					crytools.LookAndWrite obj jShif
				)
				else
				(
					cryTools.LookAndDelete obj "shift"
				)
			)
			else
			(
				cryTools.LookAndDelete obj "limit"
				cryTools.LookAndDelete obj "twist"
				cryTools.LookAndDelete obj "bend"
				cryTools.LookAndDelete obj "pull"
				cryTools.LookAndDelete obj "push"
				cryTools.LookAndDelete obj "shift"
			)
			------------Destroyable Objects------------
			if isMain.checked == true then obj.name = "Main"
			if isRemain.checked == true then obj.name = "Remain"
			
			if isEntity.checked == true then
			(
				crytools.LookAndWrite obj "entity"
			)
			else
			(
				crytools.LookAndDelete obj "entity"
			)
			
			if xrot.checked == true then
			(
				AxisRotations = "rotaxes = x"
				if yrot.checked == true then
				(
					AxisRotations = "rotaxes = xy"
					if zrot.checked == true then
					(
						AxisRotations = "rotaxes = xyz"
					)
				)
				else
				(
					if zrot.checked == true then
					(
						AxisRotations = "rotaxes = xz"
					)
				)
				crytools.LookAndWrite obj AxisRotations
			)
			
			if yrot.checked == true then
			(
				AxisRotations = "rotaxes = y"
				if xrot.checked == true then
				(
					AxisRotations = "rotaxes = xy"
					if zrot.checked == true then
					(
						AxisRotations = "rotaxes = xyz"
					)
				)
				else
				(
					if zrot.checked == true then
					(
						AxisRotations = "rotaxes = yz"
					)
				)
				crytools.LookAndWrite obj AxisRotations
			)
			
			if zrot.checked == true then
			(
				AxisRotations = "rotaxes = z"
				if yrot.checked == true then
				(
					AxisRotations = "rotaxes = yz"
					if xrot.checked == true then
					(
						AxisRotations = "rotaxes = xyz"
					)
				)
				else
				(
					if xrot.checked == true then
					(
						AxisRotations = "rotaxes = xz"
					)
				)
				crytools.LookAndWrite obj AxisRotations
			)
			if xrot.checked ==false and yrot.checked==false and zrot.checked==false then
			(
				crytools.LookAndDelete obj "rotaxes"
			)
			
			if sizeVar.value != 0 then
			(
				Var = ("sizevar = " + sizeVar.value as string)
				crytools.LookAndWrite obj Var
			)
			else
			(
				crytools.LookAndDelete obj "sizevar"
			)
			
			if  GenericCount.value != 0 then
			(
				GenCount = ("generic = " + GenericCount.value as string)
				crytools.LookAndWrite obj GenCount
			)
			else
			(
				crytools.LookAndDelete obj "generic"
			)
		)
	)
	
	on useJoint changed state do
	(
		for obj in selection do
		(
			if (classof obj) != Dummy then
			(
				if (classof obj) != Point then
				(
					messagebox "Only Helper objects can be joints\nThe selected objects are not all Dummy or Point helpers"
					useJoint.checked = false
					return undefined
				)
			)
		)
		if useJoint.checked == true then
		(
			jointLimit.enabled = true
			jointTwist.enabled = true
			jointBend.enabled = true
			jointPull.enabled = true
			jointPush.enabled = true
			jointShift.enabled = true
			
			jLimit.enabled = true
			jTwist.enabled = true
			jBend.enabled = true
			jPull.enabled = true
			jPush.enabled = true
			jShift.enabled = true
			jScale.enabled = true

		)
		else
		(
			jointLimit.enabled = false
			jointTwist.enabled = false
			jointBend.enabled = false
			jointPull.enabled = false
			jointPush.enabled = false
			jointShift.enabled = false
			
			jLimit.enabled = false
			jTwist.enabled = false
			jBend.enabled = false
			jPull.enabled = false
			jPush.enabled = false
			jShift.enabled = false
			jScale.enabled = false

		)
	)
	
	
	------PICKABLE GENERAL PROPERTY------
	on isPickable changed state do
	(
		if ispickable.checked == true then
		(
			for obj in selection do
			(
				
			)
		)
	)
	
	---------DESTROYABLE OBJECTS-------------
	on isMain changed state do
	(
		MainPresent = false
		for x in objects do
		(
			if x.name == "Main" then MainPresent = true
		)
		if MainPresent == true then 	
		(
			messagebox "There can only be one Main object"
			isMain.checked = false
		)
		if isMain.checked == true then isRemain.checked=false
	)
	on isRemain changed state do
	(
		RemainPresent = false
		for x in objects do
		(
			if x.name == "Remain" then RemainPresent = true
		)
		if RemainPresent == true then 	
		(
			messagebox "There can only be one Remain object"
			isRemain.checked = false
		)
		if isRemain.checked == true then isMain.checked=false
	)

	
	on isBone changed state do
	(
		if isBone.checked == true then
		(
			isBoneTXT.enabled = true
		)
		else
		(
			isBoneTXT.enabled = false
		)
	)
	
	---------------------------------------------
	-------CGF METADATA LISTER---------
	---------------------------------------------
	
	/*on openMetaDataLister pressed do
	(
		rollout listview_rollout "CGF MetaData Lister"
		(	
			fn initListView lv =
			(
				lv.gridLines = true  
				lv.View = #lvwReport  
				lv.fullRowSelect = true 
				
				layout_def = #("Node","Mass","Density","Primitive","Joint","Limit,Twist,Bend,Pull,Push,Shift")
				
				for i in layout_def do
				(
					column = lv.ColumnHeaders.add() 
					column.text = I
					column.width = 1500
					if (column.text = I) == "Limit,Twist,Bend,Pull,Push,Shift" then
					(
						column.width = 4300
					)
				) 
			) 
			
			fn fillInSpreadSheet lv =
			(
				for o in selection do
				(
					li = lv.ListItems.add()
					li.text = o.name
					sub_li = li.ListSubItems.add()
					sub_li.text = (crytools.getUDP o "mass") as string
					sub_li = li.ListSubItems.add()
					sub_li.text = (crytools.getUDP o "density") as string
					sub_li = li.ListSubItems.add()
					sub_li.text = try((o.mesh.numfaces) as string)catch("--")
					sub_li = li.ListSubItems.add()
					sub_li.text = (o.material) as string
				)
			) 
			
			activeXControl lv_objects "MSComctlLib.ListViewCtrl" width:790 height:190 align:#center 
			on listview_rollout open do 
			(
				initListView lv_objects
				fillInSpreadSheet lv_objects
			) 
		)
		
		try(destroyDialog listview_rollout)catch()
		createDialog listview_rollout 800 200
	)*/
)

addRollout ArtistTools CryArtistTools
addRollout Diagnostics CryArtistTools rolledup:true
addRollout metadataManager CryArtistTools rolledup:true
)


