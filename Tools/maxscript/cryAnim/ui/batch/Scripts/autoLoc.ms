
struct autoLocStruct
(
	setCon,
	getChildExtent,
	getMaxValue,
	setBodyMass,
	calcLoc,
	setLoc
)


autoLoc = autoLocStruct()


autoLoc.setCon = function setCon node time =
(
	try
	(
		for i = 1 to node.controller.keys.count do
		(
			try
			(
				tempKey = biped.getKey node.controller i
				if tempKey.time == time then
				(
					tempKey.continuity = 0
				)
			)
			catch()
		)
	)
	catch()
)
setCon = undefined



autoLoc.calcLoc = function calcLoc =
(
	cycleLoc = true
	rotateLoc = #none
	posChange = false
	startLoc = #none
	locoCycle = false
	
	
	
	at time animationRange.start
	(
		startRot = $Bip01.transform.rotation as eulerangles
		startPos = $Bip01.transform.pos
		
		startLFootPos = $'Bip01 L Toe0'.transform.pos
		startRFootPos = $'Bip01 R Toe0'.transform.pos
	)
	
	at time animationRange.end
	(
		endRot = $Bip01.transform.rotation as eulerangles
		endPos = $Bip01.transform.pos
	)
	
	at time ((animationRange.start + animationRange.end) / 2)
		midRot = $Bip01.transform.rotation as eulerangles
	
	at time 2f
	(
		startMidPos = $Bip01.transform.pos
		
		startMidLFootPos = $'Bip01 L Toe0'.transform.pos
		startMidRFootPos = $'Bip01 R Toe0'.transform.pos
	)
	
	at time (animationRange.end - 2)
		endMidPos = $Bip01.transform.pos
	
	

	diffRot = #(endRot.x, endRot.y, endRot.z)
	diffRot[1] -= startRot.x
	diffRot[2] -= startRot.y
	diffRot[3] -= startRot.z
	
	diffMidRot = #(midRot.x, midRot.y, midRot.z)
	diffMidRot[1] -= startRot.x
	diffMidRot[2] -= startRot.y
	diffMidRot[3] -= startRot.z
	
	
	
	diffPos = endPos - startPos	
	diffStartPos = startMidPos - startPos
	diffEndPos = endPos - endMidPos
	
	diffLFootPos = startLFootPos - startMidLFootPos
	diffRFootPos = startRFootPos - startMidRFootPos
	
	diffRotPositiv = copy diffRot #noMap
	
	for i = 1 to 3 do
	(
		if diffPos[i] < 0 then diffPos[i] *= -1
		if diffPos[i] > 10 then posChange = true
		if diffRotPositiv[i] < 0 then diffRotPositiv[i] *= -1
		if diffRotPositiv[i] > 2 then cycleLoc = false	
		--if diffMidRot[i] < 0 then diffMidRot[i] *= -1
		
		if diffLFootPos[i] < 0 then diffLFootPos[i] *= -1
		if diffLFootPos[i] > 10 then locoCycle = true
		
		if diffRFootPos[i] < 0 then diffRFootPos[i] *= -1			
		if diffRFootPos[i] > 10 then locoCycle = true
	)
	
	
	
	if cycleLoc == false then locoCycle = false 
	
	
	diffStartPos = distance startPos startMidPos
	diffEndPos = distance endMidPos endPos
		
	
	
	if diffStartPos < 5 and diffStartPos > 0.4 then
	(
		if locoCycle == false then
			startLoc = #start
	)
	if diffStartPos > 4 and diffEndPos < 2 then
		if locoCycle == false then
			if cycleLoc == false then
				startLoc = #stop	
	
	
	
	
	if diffRot[3] < 140 and diffRot[3] > 80 then
		rotateLoc = #left
	if diffRot[3] < -40 and diffRot[3] > -100 then
		rotateLoc = #right
	
	
	if diffMidRot[3] > -220 and diffMidRot[3] < -140 then	
	(
		if diffRot[3] < -140 and diffRot[3] > -180 then
			rotateLoc = #revL
	)
	
	if diffMidRot[3] > -140 and diffMidRot[3] < -60 then	
	(
		if diffRot[3] < -130 and diffRot[3] > -170 then
			rotateLoc = #revR
	)

--	print ("diffMidRot = " + diffMidRot as String)
--	print ("diffRot = " + diffRot as String)
--	print ("diffEndPos = " + diffEndPos as String)
--	print ("diffStartPos = " + diffStartPos as String)
--	print ("diffLFootPos = " + diffLFootPos as String)
--	print ("diffRFootPos = " + diffRFootPos as String)
--	print ("startLoc = " + startLoc as String)
--	print ("rotateLoc = " + rotateLoc as String)
--	print ("cycleLoc = " + cycleLoc as String)
--	print ("posChange = " + posChange as String)
--	print ("locoCycle = " + locoCycle as String)
	
	
	struct locStruct ( start, rotate, cycle, position )
	local tempValue = locStruct start:startLoc rotate:rotateLoc cycle:cycleLoc position:posChange
	
--	print tempValue
		
	return tempValue
)
calcLoc = undefined


autoLoc.setLoc = function setLoc =
(
	analyseLoc = autoLoc.calcLoc()
	
	cryTools.cryAnim._f.resetLocator()
	
	cryTools.cryAnim._f.moveToBodyMass()
	
	if analyseLoc.rotate != #none then
	(
		at time 10f
		(
			biped.setTransform $Locator_Locomotion #pos $Locator_Locomotion.transform.pos true
			autoLoc.setCon $Locator_Locomotion 10f
		)
		
		at time animationRange.end
			cryTools.cryAnim._f.moveToBodyMass()
		
		
		with animate on
		(
			at time 16f
			(
				case analyseLoc.rotate of
				(
					#left: rotate $Locator_Locomotion (eulerangles 0 0 89)
					#right: rotate $Locator_Locomotion (eulerangles 0 0 -89)
					#revL: rotate $Locator_Locomotion (eulerangles 0 0 179)
					#revR: rotate $Locator_Locomotion (eulerangles 0 0 -179)
				)
				autoLoc.setCon $Locator_Locomotion 16f
			)
			
			at time animationRange.end
			(
				case analyseLoc.rotate of
				(
					#left: rotate $Locator_Locomotion (eulerangles 0 0 89)
					#right: rotate $Locator_Locomotion (eulerangles 0 0 -89)
					#revL: rotate $Locator_Locomotion (eulerangles 0 0 179)
					#revR: rotate $Locator_Locomotion (eulerangles 0 0 -179)
				)	
			)
		)
	)
	else
	(
		if analyseLoc.position == true then
		(
			if analyseLoc.start == #start then
			(
				at time 10f
				(
					biped.setTransform $Locator_Locomotion #pos $Locator_Locomotion.transform.pos true
					autoLoc.setCon $Locator_Locomotion 10f
				)
				
				at time animationRange.end
					cryTools.cryAnim._f.moveToBodyMass()
			)
			else
			(
				at time animationRange.end
					cryTools.cryAnim._f.moveToBodyMass()
			)
		)
		else
		(
			if analyseLoc.start == #start then
				at time animationRange.end
					cryTools.cryAnim._f.moveToBodyMass()
		)
	)
	
	sliderTime = animationRange.start + 1
	sliderTime = animationRange.start
	
)
setLoc = undefined

try
	autoLoc.setLoc()
catch
	cryTools.cryAnim.UI.batchProcess._v.customScriptStatus = "Error: Auto-Loc"




	
	
	