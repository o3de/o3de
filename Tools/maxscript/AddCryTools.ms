-------------------------------------------------------------------------------
-- AddCryTools.ms
-- Version 2.0
-- Adds the current CryTools menu
-------------------------------------------------------------------------------



try (cryTools.cryAnim.base.killCryAnim() ) catch()

-------------------------------------------------------------------------------
--	Set the cTools struct
-------------------------------------------------------------------------------

struct cTools
(
	
	--global storage arrays
	_fnStore, _fnStoreIndex, _varStore, _varStoreIndex,
	
	localBuildNumber, BuildPathFull, perforcePath, maxDirTxt, editorPath, cryINI, maxVersionNum, project_name,
	latest_build, latestbuildnumber, cryExportPresent, cbaPath, rollback_status, DOMAIN,
	
	cryTemp = (sysInfo.tempDir + "cry_temp\\"),
	
	--INI read in
	showSplash, reparenttwist, checkbeforeexport, suppresswarnings, warnmats, nounparentw, texturesOK, updateCollections,
	syncCollections, udpObj, loadoldanimtools, generateMenu, -- added new menu generating function
	
	--Check For Export Variables
	CheckSceneForExport, PreExportWarnings, CheckBnScale, ReparentTwistBns, CheckObjMat, WpnBnUnlink, 
	CheckNodeTm, CheckNonZeroMorphs, CheckVerts,
	
	--functions
	scmd, getPerforcePath, existFile, existDir, storeFn, retrieveFn, getDNS, outToINI, inFromINI, md5, alienBrain, MirrorObjs,
	matchPivot, findRoot, getBindPoseVertexTarget, CreateBindPoseMorph, getChildren, getBips, inFromUDP, outToUDP, storeVar, retrieveVar,
	local2unc, forceLowerCase, getUDP, plusR, minusR, objTrajectoryToSpline, cutString, vertDistance, copyPasteController,
	sliderMan, rotBind, polyStats, closestPoint, nodes2bones, readIN, writeOUT, distDiff, lookAtPos, threePlaneInter, avgArray, generateMenu, setupMacros, setupVars, setupCallbacks,
	setupPlugins, setupTools, cryAnim, LookAndWrite, LookAndDelete, sortRootChildren
	
)


-- create instance of ctools struct
cryTools = cTools()








	
	-- Silent command
cryTools.scmd = fn scmd command wait =
(
	csexport.export.execute_command_line ("cmd /c \"" + command + "\"") wait
)
scmd = undefined



--// retrieves perforce path of the project if perforce is available
cryTools.getPerforcePath = fn getPerforcePath =
(
	perforceString = ""
	tempFile = ((getDir #maxroot) + "perforceDir.ct")
	
	local buildFilter = filterString buildPathFull_crytools "\\"
	for i = 1 to buildFilter.count do
		perforceString += buildFilter[i] + (if i < buildFilter.count then "\\" else "")
	
	
	if cryTools.scmd != undefined then
	(
		cryTools.scmd ("p4 where " + perforceString + " > \"" + tempFile + "\"") true
		
		tempString = ""		
		while tempString == "" do
		(
			escapeEnable = true
			
				--// checks if the file is generated
			while tempStream == undefined do
			(
				escapeEnable = true
				tempStream = openFile tempFile mode:"r"
			)
			
			if getFileSize tempFile == 0 then
			(
				print "Perforce not found..."
				return undefined
			)
			
			while eof tempStream != true do
				tempString += (readLine tempStream) + "\n"
			
			close tempStream
				--// delete file
			deleteFile tempFile
		)
		
		try ( return (filterString tempString " ")[1] ) catch (return undefined)
	)
)
getPerforcePath = undefined


--	File/Dir exist functions
cryTools.existFile = fn existFile fname = return ((getfiles fname).count != 0)
existFile = undefined


cryTools.existDir = fn existDir fname = return ((getdirectories fname).count != 0)
existDir = undefined


--reading and writing to the crytools.fnStore array
cryTools.storeFn = fn storeFn storeMe overWrite = 
(
	if (finditem cryTools._fnStoreIndex (storeMe as string)) == 0 then
	(
		--print ("Cannot find " + storeme as string + " in " + crytools.fnstore as string)
		append cryTools._fnStore storeMe
		append cryTools._fnStoreIndex (storeMe as string)
	)
	else
	(
		if overWrite == true then
		(
			itemIndex = (finditem cryTools._fnStoreIndex (storeMe as string))
			cryTools._fnStore[itemIndex] = storeMe
			cryTools._fnStoreIndex[itemIndex] = (storeMe as string)
		)
		else
		(
			print "function exists"
		)
	)
)
storeFn = undefined


cryTools.retrieveFn = fn retrieveFn name =
(
	index = finditem crytools._fnStoreIndex name
	if index == 0 then
		return undefined
	else
		return crytools._fnStore[index]
)
retrieveFn = undefined

--reading and writing to the crytools.fvarStore array
cryTools.storeVar = fn storeVar storeMe alias overWrite = 
(
	if (finditem crytools._varStoreIndex alias) == 0 then
	(
		append crytools._varStore storeMe
		append crytools._varStoreIndex alias
	)
	else
	(
		if overWrite == true then
		(
			itemIndex = finditem crytools._varStoreIndex alias
			crytools._varStore[itemIndex] = storeMe
			crytools._varStoreIndex[itemIndex] = alias
			print "overwritten"
		)
		else
		(
			print "alias exists"
		)
	)
)
storeVar = undefined



cryTools.retrieveVar = fn retrieveVar name =
(
	index = finditem crytools._varStoreIndex name
	if index == 0 then
		return undefined
	else
		return crytools._varStore[index]
)
retrieveVar = undefined



--	Get DNS info
cryTools.getDNS = fn getDNS =
(
	try
	(
		cryTools.scmd ("ipconfig /all > " + crytools.cryTemp + "ipconfig.txt") true
		in_text = openfile (crytools.cryTemp + "ipconfig.txt")
		skipToString in_text "Primary Dns Suffix  . . . . . . . : "
		line = readline in_text
		close in_text
		deleteFile (crytools.cryTemp + "ipconfig.txt")
		return line
	)
	catch ( return "UNDEFINED" )
)
getDNS = undefined



cryTools.outToINI = fn outToINI subject key data =
(
	csexport.set_value key data
)
outToINI = undefined



cryTools.writeOUT = fn writeOUT data path echo:undefined = 
(
	if doesfileexist path == false then
	(
		INIFile = createFile path
		for i = 1 to data.count do
		(
			if echo == true then 
			(
				print data[i]
			)
			format (data[i] + "\n") to:INIFile
		)
		close INIfile
	)
	else
	(
		try
		(
			INIfile = openfile path mode:"w"
		)
		catch
		(
			cryTools.minusR path
		)
		INIfile = openfile path mode:"w"
		for i = 1 to data.count do
		(
			if echo == true then 
			(
				print data[i]
			)
			format (data[i] + "\n") to:INIFile
		)
		close INIfile
	)
)
writeOUT = undefined



cryTools.inFromINI = fn inFromINI subject key =
(
	return (csexport.get_value key)
)
inFromINI = undefined



cryTools.readIN = fn readIN path echo:undefined =
(
	if doesfileexist path == false then
	(
		print ("file " + path + " does not exist!")
		return undefined
	)
	INIfile = openfile path mode:"r"
	fileContents = #()
	while not EOF INIfile do
	(
		append fileContents (readline INIfile)
	)
	close INIfile
	if echo == true then
	(
		print fileContents
	)
	return fileContents
)
readIN = undefined

	--// for old ini version
try ( csexport.set_value )
catch
(
	cryTools.outToINI = fn outToINI subject key data =
	(
		setINISetting (sysInfo.tempDir + "cry_temp\\cryTools.ini") subject key data
	)
	outToINI = undefined

	
	cryTools.inFromINI = fn inFromINI subject key =
	(
		return (getINISetting (sysInfo.tempDir  + "cry_temp\\crytools.ini") subject key)
	)
	inFromINI = undefined
)




--	MD5 hash return
cryTools.md5 = fn md5 md5path =
(
	cryTools.scmd ("\"" + cryTools.BuildPathFull + "Tools\\maxscript\\md5\" \"" + md5path + "\" > " + ("\"" + sysInfo.tempDir + "cry_temp\\hash.txt\"")) true
	if doesfileexist md5path == false then
	(
		print ( md5path + " does not exist.")
		return undefined
	)
	md5hashfile = openFile (sysInfo.tempDir + "cry_temp\\hash.txt")
	while not eof md5hashfile do
	(
		md5hashline = (readline md5hashfile)
	)
	try
	(
		md5hashArray = (filterstring md5hashline " ")
	)
	catch
	(
		return ("INVALID PATH: " + md5path)
	)
	close md5hashfile
	deleteFile (sysInfo.tempDir + "cry_temp\\hash.txt")
	return md5hashArray[1]
)
md5 = undefined

-- AlienBrain interface via command line execution
cryTools.alienBrain = fn alienBrain abcommand =
(
	crytools.scmd abcommand true
)
alienBrain = undefined




--	For mirroring joint orientations over an arbitrary axis
cryTools.MirrorObjs = fn MirrorObjs Obj1 Obj2 MObj MAxis OAxis =
(
	T = Obj2.Transform * inverse MObj.Transform --- the offset transform Matrix
	Case MAxis of --- Mirror T in one axis of MObj 
	(
		#x:( for i = 1 to 4 do T[i] = [ -T[i].x , T[i].y , T[i].z ] )
		#y:( for i = 1 to 4 do T[i] = [ T[i].x , -T[i].y , T[i].z ] )
		#z:( for i = 1 to 4 do T[i] = [ T[i].x , T[i].y , -T[i].z ] )
	)
	case OAxis of --- But the axis has been flipped ...so flip it back.
	(
		#x:( T[1] = - T[1] )
		#y:( T[2] = - T[2] )
		#z:( T[3] = - T[3] )
	)
 
	TM = T * MObj.Transform 
	Obj1.rotation = Conjugate TM.rotation
	Obj1.pos = TM.pos
)
MirrorObjs = undefined



--	Aligning the pivot of an object to that of another
cryTools.matchPivot = fn matchPivot obj1 obj2 =
(
	obj2.pivot = obj1.pivot
	pivrot = obj1.rotation
	in coordsys local obj2.rotation*=pivrot
	obj2.objectoffsetrot*=pivrot
	obj2.objectoffsetpos*=pivrot
)
matchPivot = undefined



--	Gets the root of the hierarchy, when you give it an object of said hierarchy.
cryTools.findRoot = fn findRoot obj =
(
	try
	(
		if obj.parent == undefined then
		(
			return obj
		)
		local p1 = obj
		for i = 1 to 10000 do
		(
			p2 = p1.parent
			if p2 == undefined then return p1 else p1 = p2
		)
	)
	catch( return obj)
)
findRoot = undefined




--	Smithy's badassed morph extraction
cryTools.getBindPoseVertexTarget = fn getBindPoseVertexTarget targetNode sourceNode vertexID =
(
	-- Find the skin modifier.
	skin = sourceNode.skin

	-- Loop through all the bones influencing this vertex.
	-- For now just use one of the bones.
	skinTransform = Matrix3 0
	for link = 1 to skinOps.getVertexWeightCount skin vertexID do
	(
		-- Get the bone that the vertex is linked to.
		boneID = skinOps.getVertexWeightBoneID skin vertexID link
		boneName = skinOps.GetBoneName skin boneID 0
		bone = getNodeByName boneName

		-- Find the bind pose transform of the bone.
		in coordsys world bindPoseTM = skinUtils.GetBoneBindTM sourceNode bone

		-- Find the current transform of the bone.
		in coordsys world currentTM = bone.transform

		-- Find the relative transform.
		bindPoseToCurrentTM =  (inverse bindPoseTM) * currentTM

		-- Get the weight assigned to this bone.
		weight = skinOps.GetVertexWeight skin vertexID link
		scale bindPoseToCurrentTM [weight, weight, weight] true

		-- Compute the weighted sum of the transform matrix.
		skinTransform = skinTransform + bindPoseToCurrentTM
	)

	-- Apply the inverse of this transform to the vertex, to return to bind pose.
	bindPoseVertex = (getVert targetNode vertexID) * (inverse skinTransform)

	return bindPoseVertex
)
getBindPoseVertexTarget = undefined




--	CreateBindPoseMorph
cryTools.CreateBindPoseMorph = fn CreateBindPoseMorph targetNode sourceNode deleteYN addMorphYN channelNum extractedNode =
(	
	-- Make sure the user has selected the skinned character.
		if targetNode != undefined then
		(
			targetCopy = copy targetNode name:extractedNode
			convertToMesh targetCopy

			targetCopy.transform = sourceNode.transform

			-- Transform all vertices to the bind pose.
			for vertex = 1 to (getNumVerts targetCopy) do
			(
				bindPosePosition = getBindPoseVertexTarget targetCopy sourceNode vertex
				meshOp.setVert targetCopy vertex bindPosePosition
			)
			
			-- Add the morph channel.
			if addMorphYN == 1 then
				(
				WM3_MC_Delete sourceNode.morpher channelNum
				wm3_mc_buildfromnode sourceNode.morpher channelNum targetCopy
				)
			
			-- Delete the cloned morph target.
			if deleteYN == 1 then
				(
				delete targetCopy
				)
		)
)
CreateBindPoseMorph = undefined




-- Get children
cryTools.getChildren = fn getChildren theNode =
(
	nodeArray = #()
	for obj in theNode.children do
	(
		append nodeArray obj
		join nodeArray (cryTools.getChildren obj)
	)
	return nodeArray
)
getChildren = undefined


	
--function to get all bip roots
cryTools.getBips = fn getBips =
(
	Bips = #()
	for obj in geometry do
	(
		if classof obj.controller == Vertical_Horizontal_Turn then 
		(
			append Bips obj
		)
	)
	return Bips
)
getBips = undefined




--functions for reading and writing to user defined properties
cryTools.inFromUDP = fn inFromUDP obj =
(
	if obj == undefined then
	(
		print "No Object is Selected!"
		return undefined
	)
	UDPin = stringstream (getUserPropBuffer obj)
	udpContents = #()
	while not eof UDPin do
	(
		append udpContents (readline (UDPin))
	)
	for i = 1 to udpContents.count do
	(
		if udpContents[i] == "" then
		(
			deleteItem udpContents i
		)
	)
	return udpContents
)
inFromUDP = undefined




cryTools.outToUDP = fn outToUDP data obj echo =
(
	text = ""
	
	if obj == undefined then
	(
		print ("outToUDP: " + obj.name + " does not exist!")
		return undefined
	)
	
	
	for i = 1 to data.count do
		text += data[i] + "\r\n"
	
	
	if echo == true then
		print text
	
	setUserPropBuffer obj text
)
outToUDP = undefined



cryTools.LookAndDelete = fn LookAndDelete obj word=
(
	local ObjUDP = crytools.inFromUDP obj
	local index = 0
	
	if ObjUDP.count > 0 then
	(
			--// goes through udp entries
		for i = 1 to ObjUDP.count do
		(
				--// if the word is found in the entry
			if word == ((filterstring ObjUDP[i] " =")[1]) then
			(
					--// exit loop
				index = i
				exit
			)
		)
		
		if index != 0 then
				--// delete entry
			deleteItem ObjUDP index
		
			--// set new udp entries
		crytools.outtoudp ObjUDP obj false
	)
)
LookAndDelete = undefined



cryTools.LookAndWrite = fn LookAndWrite obj data =
(
	local ObjUDP = crytools.inFromUDP obj
	local TestArr = #()
	local wordLocation = 0
	
	for i = 1 to ObjUDP.count do
	(
			--// adds the entries name to the test array
		append TestArr (filterstring ObjUDP[i] " =")[1]
			--// if the entry is the same as data
		if TestArr[i] == (filterString data " =")[1] then
				--// set new location in the array
			wordLocation = i
	)
	
		--// tries to find the word in the entries list, if it can't find it, set location to the next of the last entry
	if wordLocation == 0 then wordLocation = ObjUDP.count + 1
	
		--// write new entry
	ObjUDP[wordLocation] = data
	crytools.OutToUDP  ObjUDP obj false
)
LookAndWrite = undefined



cryTools.local2unc = fn local2unc letter =
(
	cmd = ("net use " + letter + ": > " + crytools.cryTemp + "local_unc.txt")
	crytools.scmd cmd true
	loadIn = openfile (crytools.cryTemp + "local_unc.txt")
	skipToString loadIn "Remote"
	loadInARR = (filterString (readline loadIn) " ")
	close loadIn
	cmd = ("del " + crytools.cryTemp + "local_unc.txt")
	crytools.scmd cmd true
	return loadInARR[2]
)
local2unc = undefined




cryTools.forceLowerCase = fn forceLowerCase inputString =
(
	charCache = #()
	outputString = ""
	inputStream = (inputString as stringStream)
	while not eof inputStream do
	(
		append charCache (readChar inputStream) 
	)
	for i = 1 to charCache.count do
	(
		case charCache[i] of
		(
			"A": charCache[i] = "a"
			"B": charCache[i] = "b"
			"C": charCache[i] = "c"
			"D": charCache[i] = "d"
			"E": charCache[i] = "e"
			"F": charCache[i] = "f" 
			"G": charCache[i] = "g"
			"H": charCache[i] = "h"
			"I": charCache[i] = "i"
			"J": charCache[i] = "j" 
			"K": charCache[i] = "k"
			"L": charCache[i] = "l"
			"M": charCache[i] = "m"
			"N": charCache[i] = "n" 
			"O": charCache[i] = "o"
			"P": charCache[i] = "p"
			"Q": charCache[i] = "q"
			"R": charCache[i] = "r" 
			"S": charCache[i] = "s"
			"T": charCache[i] = "t"
			"U": charCache[i] = "u"
			"V": charCache[i] = "v" 
			"W": charCache[i] = "w"
			"X": charCache[i] = "x"
			"Y": charCache[i] = "y"
			"Z": charCache[i] = "z" 
		)
	)
	for i = 1 to charCache.count do
	(
		append outputString charCache[i]
	)
	return outputString
)
forceLowerCase = undefined




cryTools.getUDP = fn getUDP obj attrib =
(
		--// init vars
	returnVal = ""
	udpInARR = crytools.inFromUDP obj	
	
		--// go through every udp entry
	for i = 1 to udpInARR.count do
	(
			--// get's name
		local testFor = (cryTools.forceLowerCase (filterString udpInArr[i] " =")[1])
			--// get's value
		local testValue
		if (filterString udpInArr[i] " =")[2] != undefined then
		(
			testValue = ((filterString udpInArr[i] " =")[2] as float)
		)
		else
		(
			testValue = true
		)
			--// returns value, if entry is the same as attrib
		if testFor == attrib then
			return testValue
	)
	return undefined
)
getUDP = undefined




-- file attrib +/-r in explorer
cryTools.plusR = fn plusR path = 
(
	if doesfileexist path == true then
	(
		crytools.scmd ("attrib +r \"" + path + "\"") true
	)
	else
	(
		print (path + " does not exist")
	)
)
plusR = undefined




cryTools.minusR = fn minusR path = 
(
	if doesfileexist path == true then
	(
		cryTools.scmd ("attrib -r \"" + path + "\"") true
	)
	else
	(
		print (path + " does not exist")
	)
)
minusR = undefined




cryTools.objTrajectoryToSpline = fn objTrajectoryToSpline =
(
	for obj in selection do
	(
		ss = SplineShape pos:(obj.position.keys[1].value) name:("traj_" + obj.name)
		addNewSpline ss
		for i = 2 to obj.position.keys.count do
		(
			addKnot ss 1 #corner #line obj.position.keys[i].value
		)
		updateShape ss
		ss.transform = obj.transform
	)
)
objTrajectoryToSpline = undefined




cryTools.cutString = fn cutString stringIn cut =
(
	startCut = (findString stringIn cut)
	if startCut == undefined then
	(
		print ("error: '" + stringIn + "' '" + cut +"'")
		return stringIn
	)
	return (replace stringIn startCut cut.count "")
)
cutString = undefined




cryTools.vertDistance = fn vertDistance pos1 pos2 =
(
	distX = (pos1.x - pos2.x) * (pos1.x - pos2.x)
	distY = (pos1.y - pos2.y) * (pos1.y - pos2.y)
	distZ = (pos1.z - pos2.z) * (pos1.z - pos2.z)
	
	hypotDistXYZ = sqrt(distX + distY + distZ) as float
	
	return hypotDistXYZ 
)
vertDistance = undefined




cryTools.copyPasteController = fn copyPasteController parentObj selOrChild what = 
(
	if selOrChild == "children" then
	(
		for obj in parentObj.children do
		(
			obj.parent = undefined
			
				--// if p exists
			if (findString what "p") != undefined then
				obj.position.controller = parentObj.position.controller
				--// if r exists
			if (findString what "r") != undefined then
				obj.rotation.controller = parentObj.rotation.controller
		)
	)
)
copyPasteController = undefined




--old test, just puts in manip mode if slider is selected (needs callback)
cryTools.sliderMan = fn sliderMan =
(
	if selection != undefined then
	(
		if classof $ == sliderManipulator then
		(
			manipulateMode = true
		)
		else
		(
			manipulateMode = false
		)
	)
)
sliderMan = undefined

cryTools.rotBind = fn rotBind mesh root x y z leaveSkel =
(
	select mesh
	
	--place bip into fig mode
	if root.name == "Bip01" then
	(
		$Bip01.controller.figureMode = true
	)
	
	--save out weights
	max modify mode
	try 
	(
		modPanel.setCurrentObject mesh.modifiers[#Skin]
	)
	catch
	(
		print (mesh.name + " does not appear to have a Skin modifier.")
		return undefined
	)
	skinOps.SaveEnvelope mesh.modifiers[#Skin] (crytools.cryTemp + "tests.env")
	local boneXfer = #()
	for i=1 to (skinOps.getNumberBones mesh.skin) do
	(
		append boneXfer (skinOps.GetBoneName mesh.skin i 0)
	)
	
	--flip the character mesh
	collapseStack mesh
	mesh.transform[4] = root.transform[4]
	in coordsys world rotate mesh (eulerangles x y z)
	ResetXForm mesh
	collapseStack mesh
	
	--flip the skeleton
	in coordsys world rotate root (eulerangles x y z)
	
	--readd skin to the mesh
	addModifier mesh (Skin ())
	modPanel.setCurrentObject mesh.modifiers[#Skin]
	for i = 1 to (boneXfer.count - 1) do
	(
		skinOps.addbone mesh.modifiers[#Skin] (getnodebyname (bonexfer[i])) 0
	)
	skinOps.addbone mesh.modifiers[#Skin] (getnodebyname (bonexfer[bonexfer.count])) 1
	--don't ask me why you have to do this twise to get it to work
	skinOps.LoadEnvelope mesh.modifiers[#Skin] (crytools.cryTemp + "tests.env")
	skinOps.LoadEnvelope mesh.modifiers[#Skin] (crytools.cryTemp + "tests.env")
	
	--flip the skeleton back
	if leaveSkel == false then
	(
		in coordsys world rotate root (eulerangles -x -y -z)
	)
)
rotBind = undefined

crytools.polyStats = fn polyStats =
(
	try
	(
		if selection.count == 0 then
		(
			return #(0,0,0)
		)
		
		totalFace = 0
		totalEdge = 0
		totalVert = 0
		
		for obj in selection do
		(
			totalFace += obj.mesh.numfaces
			totalEdge += obj.mesh.edges.count
			totalVert += obj.mesh.numverts
		)
		
		return #(totalFace,totalEdge,totalVert)
	)
	catch(return #(0,0,0))
)
polystats = undefined

crytools.closestpoint = fn closestpoint point1 obj =
(
	try
	(
		local posArr = #()
		for i = 1 to (polyOp.getNumVerts obj) do
		(
			local vPos = polyOp.getVert obj i
			append posArr vPos
		)
		local sPosArr = #()
		for i in posArr do
		(
			local distX = distance point1 i
			append sPosArr distX
		)
		local minDistance = amin sPosArr
		local minVert = findItem sPosArr (minDistance)
		return minVert
	)
	catch()
)
closestpoint = undefined

crytools.nodes2bones = fn nodes2bones node =
(
	try
	(
		if node.children != undefined then
		(
			newBones = #()
			
			for obj in (append (crytools.getchildren node) node) do
			(
				for child in obj.children do
				(
					if child.children != undefined then
					(
						test = BoneSys.createBone obj.transform.pos child.transform.pos obj.transform[3]
						test.name = (obj.name + "_bone")
						append newBones test
					)
				)
			)
			for obj in newBones do
			(
				try (obj.parent =  (getnodebyname (((getnodebyname (crytools.cutstring obj.name "_bone")).parent).name + "_bone"))) catch()
			)
		)
	)
	catch()
)
nodes2bones = undefined


--###############################################################################
	--// goes through the timeline and returns the distance largest distance two points move relative to start distance
	--// used for looking at mocap stabilization markers
--###############################################################################
crytools.distDiff = fn distDiff pos1 pos2 detail: logOut: =
(
	distMax = 0
	distMin = 100000
	local minF
	local maxF
	local stabLog = ""
	local origDist = at time animationrange.start (distance pos1 pos2)
	
	for i=animationrange.start to animationrange.end do
	(
		at time i
		(
			dist = (distance pos1 pos2)
			stabLog += (i as string+ "_" + (origDist - dist) as string + "\n")
			if dist < distMin then
			(
				distMin = dist
				minF = i
			)
			if dist > distMax then
			(
				distMax = dist
				maxF = i
			)
		)
	)
	print (distmax as string + " " + distmin as string)
	if logOut == true and detail == true then
	(
		return #((distmax-distmin),minF,maxF, stabLog)
	)
	else if detail = true then
	(
		return #((distmax-distmin),minF,maxF)
	)
	else
	(
		return (distmax-distmin)
	)
)
distDiff = undefined

cryTools.lookatPos = fn lookAtObj obj1 pos obj1axis = 
(
	local objDir
	case obj1axis of
	(
		1: objDir = obj1.transform[1]
		2: objDir = obj1.transform[2]
		3: objDir = obj1.transform[3]
	)
	theAngle = (acos(dot (normalize objDir) (normalize (pos - obj1.pos))))
	theAxis = (cross (normalize objdir) (normalize (pos - obj1.pos)))
	rotAngle = angleAxis theAngle theAxis
	rotate obj1 rotAngle
)
lookAtObj = undefined

--###############################################################################
	--// finds the intersection of three planes, returns undefined if any 2 or all 3 are parallel
--###############################################################################
crytools.threePlaneInter  = fn threePlaneInter p1 p2 p3 selectPlanes: = 
(
    local d1 = (dot p1.dir p1.pos )
    local d2 = (dot p2.dir p2.pos )
    local d3 = (dot p3.dir p3.pos )
    
    local num = ((d1 * (cross p2.dir p3.dir)) + (d2 * (cross p3.dir p1.dir)) + (d3 * (cross p1.dir p2.dir)))
    local denom = (dot p1.dir (cross p2.dir p3.dir))
    
    print ("num " + num as string)
    print ("denom " + denom as string)
    if (abs denom) < 0.0001 then (return undefined)
    
    if selectPlanes == true then
    (
        selectMe = #(p1,p2,p3)
        select selectMe
    )
    return (num / denom)
)
threePlaneInter = undefined


--###############################################################################
	--// returns an average of elements in an array
--###############################################################################
crytools.avgArray = fn avgArray arrayIn nodePos: =
(
	try (arrayIn.count)catch(return undefined)
	
	if arrayIn.count == 1 then
	(
		if nodePos == true then (return arrayIn[1].pos)else(return arrayIn[1])
	)
	
	if nodePos == true then
	(
		n = arrayIn[1].pos
		for i = 2 to arrayIn.count do
		(
			n += arrayIn[i].pos
		)
		return (n/arrayIn.count)
	)
	else
	(
		n = arrayIn[1]
		
		for i = 2 to arrayIn.count do
		(
			n += arrayIn[i]
		)
		return (n/arrayIn.count)
	)
)
avgArray = undefined


--###############################################################################
	--// sorts the array depending the hierarchy
--###############################################################################
cryTools.sortRootChildren = function sortRootChildren nodeArray =
(
	try local tempParent = getNodeByName nodeArray[1].name
	catch local tempParent = getNodeByName nodeArray[1]
	
	while tempParent.parent != undefined do
	(
		enableEscape = true
		tempParent = tempParent.parent
	)
	
	local root = tempParent
	
	nodeAllArray = #(root)
	join nodeAllArray (cryTools.getChildren root)
	
	try
	(
		for i = 1 to nodeArray.count do
			nodeArray[i] = nodeArray[i].name
	)catch()
	
	
		
	for i = 1 to nodeAllArray.count do
		nodeAllArray[i] = nodeAllArray[i].name
	
	local notParentedArray = #()
	
	
	for i = 1 to nodeAllArray.count do
	(
		if findItem nodeArray nodeAllArray[i] == 0 then
			nodeAllArray[i] = undefined
	)
	
	for i = 1 to nodeArray.count do
		if findItem nodeAllArray nodeArray[i] == 0 then
			append notParentedArray nodeArray[i]
	
	local resultArray = #()
	
	for i = 1 to nodeAllArray.count do
		if nodeAllArray[i] != undefined then
			append resultArray nodeAllArray[i]
	
	join resultArray notParentedArray
		
	return resultArray
	
)
sortRootChildren = undefined




cryTools.generateMenu = fn generateMenu =
(
	try (menuMan.unRegisterMenu (menuMan.findMenu "CryTools") ) catch()
	
	local mainMenuBar = menuMan.getMainMenuBar()
	local subMenu = menuMan.createMenu "CryTools"
	local subMenuItem = menuMan.createSubMenuItem "CryTools" subMenu
	
	mainMenuBar.addItem subMenuItem (mainMenuBar.numItems() - 1)
	menuMan.updateMenuBar()
	
	local mainMenuBar = menuMan.getMainMenuBar()
	local subMenu = menuMan.findMenu "CryTools"
	
	local addBar = menuMan.createActionItem "CryAnimation" "CryTools"
	subMenu.addItem addBar -1
	
	
	local addBar = menuMan.createActionItem "CryArtistTools" "CryTools"
	subMenu.addItem addBar -1
	local addBar = menuMan.createActionItem "CryMorphManager" "CryTools"
	subMenu.addItem addBar -1
	local addBar = menuMan.createActionItem "CryRigging" "CryTools"
	subMenu.addItem addBar -1
	local addBar = menuMan.createActionItem "CryInfo" "CryTools"
	subMenu.addItem addBar -1
	local addBar = menuMan.createActionItem "SceneBrowser" "CryTools"
	subMenu.addItem addBar -1
	
	seperator = menuMan.createSeparatorItem()
	subMenu.addItem seperator -1
	
	--local addBar = menuMan.createActionItem "UpdateTools" "CryTools"
	--subMenu.addItem addBar -1
	local addBar = menuMan.createActionItem "ControlPanel" "CryTools"
	subMenu.addItem addBar -1
	local addBar = menuMan.createActionItem "CrytoolsDocs" "CryTools"
	subMenu.addItem addBar -1
	local addBar = menuMan.createActionItem "OnlineDocumentation" "CryTools"
	subMenu.additem addBar -1

	menuMan.updateMenuBar()
)
generateMenu = undefined




cryTools.setupMacros = fn setupMacros =
(
	MacroScript UpdateTools category:"CryTools" tooltip:"Reload/Update Cry Tools"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\UpdateTools.ms")
		)
	MacroScript CryAnimation category:"CryTools" tooltip:"Animation Tools"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
			(
				try
				(
					if cryTools.loadoldanimtools == false then
					(
						if cryTools.cryAnim != undefined then
						(
							undo off
								cryTools.cryAnim.UI.main._f.callDialog()
						)
					)
					else
					(
						filein (crytools.BuildPathFull + "Tools\\maxscript\\CryAnimationTools.ms")
					)
				)catch( messageBox "Error loading cryAnimation" )
			)
		)
	MacroScript CryArtistTools category:"CryTools" tooltip:"Artist Tools"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryArtistTools.ms")
		)
	MacroScript CryMorphManager category:"CryTools" tooltip:"Facial Morph Tool"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryMorphTools.ms")
		)
	MacroScript CryRigging category:"CryTools" tooltip:"Riging Tools"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryRiggingTools.ms")
		)
	MacroScript SceneBrowser category:"CryTools" tooltip:"Scene Browser"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\SceneBrowser.ms")
		)
	MacroScript CryInfo category:"CryTools" tooltip:"Add File Info"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryInfo.ms")
		)
	MacroScript CryInfoLoader category:"CryTools" tooltip:"Auto-loads cryInfo"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryInfoLoader.ms")
		)
	MacroScript ControlPanel category:"CryTools" tooltip:"Diagnostics//Preferences"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\Diagnostics.ms")
		)
	MacroScript CrytoolsDocs category:"CryTools" tooltip:"Launch Help"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				shellLaunch (crytools.BuildPathFull + "Tools\\maxscript\\docs\\index.html") "" --Changed by Ruben "rubenst" Steinbacher, from Crytek internal path to the docs delivered with SDK
		)
	--Added by Ruben "rubenst" Steinbacher
	MacroScript OnlineDocumentation category:"CryTools" tooltip:"Launch Official CryENGINE2 Docu - Asset Creation Manual (Online)"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				shellLaunch "http://doc.crymod.com/AssetCreation/" ""
		)
	--Added by Glen Schulz:
	MacroScript CryMakeBlendShader category:"CryTools" tooltip:"Make DirectX Blend Shader from Max material"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryMakeBlendShader.ms")
		)

	--	Keybindings
	MacroScript preserveUV category:"CryKeys" tooltip:"preserveUV"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\preserveUVs.ms")
		)
	MacroScript centerPivot category:"CryKeys" tooltip:"centerPivot"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\centerPivot.ms")
		)
	MacroScript resetXformCollapse category:"CryKeys" tooltip:"resetXformCollapse"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
		
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\resetXformCollapse.ms")
		)
	MacroScript exportNodes category:"CryKeys" tooltip:"exportNodes"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\exportNodes.ms")
		)
	MacroScript exportAnim category:"CryKeys" tooltip:"exportAnim"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\exportAnim.ms")
		)
	MacroScript AddToExportList category:"CryKeys" tooltip:"AddToExportList"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\AddToExportList.ms")
		)
	MacroScript UVcollapseVertical category:"CryKeys" tooltip:"UV Collapse Vertical"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\UVcollapseVertical.ms")
		)
	MacroScript UVcollapseHorizontal category:"CryKeys" tooltip:"UV Collapse Horizontal"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\UVcollapseHorizontal.ms")
		)
	MacroScript UVscaleUniform category:"CryKeys" tooltip:"UV Scale Uniform"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\UVscaleUniform.ms")
		)
	MacroScript changeRefCoordSys category:"CryKeys" tooltip:"Change Reference Coordinate System"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\changeRefCoordsys.ms")
		)
	MacroScript showVertexColors category:"CryKeys" tooltip:"Show/Hide Vertex Colors"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\showVertexColors.ms")
		)
	MacroScript showVertexAlpha category:"CryKeys" tooltip:"Show/Hide Vertex Alpha"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\showVertexAlpha.ms")
		)
	MacroScript collapseByLoc category:"CryKeys" tooltip:"MorphTools: Collapse"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\collapseByLoc.ms")
		)
	MacroScript LimERing category:"CryKeys" tooltip:"Limited Edge/Vert Ring"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\LimERing.ms")
		)
	MacroScript LimELoop category:"CryKeys" tooltip:"Limited Edge/Vert/Poly Loop"
		(
			on isVisible do
			(
				if cryTools != undefined then
					return true
				else
					return false
			)
			
			on execute do
				filein (crytools.BuildPathFull + "Tools\\maxscript\\CryKey\\LimELoop.ms")
		)
)
setupMacros = undefined




cryTools.setupVars = fn setucpVars =
(
	crytools.maxDirTxt = maxDirTxt_crytools
	cryTools.cryINI = cryINI_crytools
	cryTools.editorPath = editorPath_crytools
	cryTools.BuildPathFull = BuildPathFull_crytools
	cryTools.perforcePath = cryTools.getPerforcePath()
	cryTools._fnStore = #()
	cryTools._fnStoreIndex = #()
	cryTools._varStore = #()
	cryTools._varStoreIndex = #()
	crytools.cryExportPresent = false
	
	cryTools.PreExportWarnings = #()

	--set vars because, well, yeah
	crytools.reparenttwist = false
	crytools.suppresswarnings = false
	crytools.warnmats = false
	crytools.nounparentw = false
	crytools.texturesOK = true
	
	if (cryTools.existDir (sysInfo.tempDir + "cry_temp")) == false then
		cryTools.scmd ("MKDIR \"" + sysInfo.tempDir + "cry_temp\"") true
	
	buildpatharray = filterstring crytools.BuildPathFull "\\"
	crytools.project_name = buildpatharray[buildPathArray.count]
	print (crytools.project_name + " is set as current project.")
	
	print "Retrieving list of latest builds from \\\\Storage\\builds"
	
	if doesfileexist "\\\\storage\\builds\\procedurally_generated_builds" == true then
	(
		if doesfileexist (crytools.BuildPathFull + "tools\maxscript\scmd.exe") != false then		
			crytools.scmd ("DIR \\\\storage\\builds\\procedurally_generated_builds\\ /B /O-D > \"" + sysInfo.tempDir + "cry_temp\\crytools.latest_builds.txt\"") true
		
		sleep .7
	)
	else
	(
		crytools.latestbuildnumber = "NET_ERROR"
		crytools.latest_build = "NET_ERROR"
	)
	
	if crytools.latestbuildnumber != "NET_ERROR" then
	(
		if doesfileexist (sysInfo.tempDir + "cry_temp\\crytools.latest_builds.txt") == true then
		(
			if doesfileexist "\\\\storage\\builds\\procedurally_generated_builds" != true then
			(
				print "Cannot find \\\\storage\\builds\\procedurally_generated_builds\\"
				return undefined
			)
			latest_build_list = openFile (sysInfo.tempDir + "cry_temp\\crytools.latest_builds.txt")
			crytools.latest_build = (readline latest_build_list)
			if crytools.latest_build == "TempBuildCopy" then
			(
				skipToNextLine latest_build_list
				crytools.latest_build = (readline latest_build_list)
			)
			if crytools.latest_build == "MP" then
			(
				skipToNextLine latest_build_list
				crytools.latest_build = (readline latest_build_list)
			)
			if crytools.latest_build == "TempBuildCopy" then
			(
				skipToNextLine latest_build_list
				crytools.latest_build = (readline latest_build_list)
			)
		buildnumberArray = filterstring crytools.latest_build "()"
		crytools.latestbuildnumber = buildnumberArray[2]
		close latest_build_list
		)
	)
	
	
	
	if (doesfileexist (sysInfo.tempDir + "cry_temp\\hash.txt")) == true then
	(
		deleteFile (sysInfo.tempDir + "cry_temp\\hash.txt")
	)
	
	
	print crytools.BuildPathFull
	if doesfileexist ((crytools.BuildPathFull + "Code_Changes.txt")) == false then
	(
		print "Code_Changes.txt cannot be found in your build directory"
	)
	else
	(
		perf_path = (crytools.BuildPathFull + "Code_Changes.txt")
		perf_changes = openFile perf_path
		skipToString perf_changes "in Build "
		local_build_line =  (readLine perf_changes)
		local_buildArray = (filterString local_build_line "-")
		crytools.localBuildNumber = local_buildArray[1]
	)
	
	cryTools.cbapath = cryTools.buildpathfull + "Game\\Animations\Animations.cba"
	
		--get the crytools.DOMAIN		
	cryTools.DOMAIN = cryTools.inFromINI "CryTools" "domain"
	if cryTools.DOMAIN == "" then (cryTools.outToINI "CryTools" "DOMAIN" (cryTools.getDNS()) ; cryTools.DOMAIN = cryTools.getDNS())
		
		--get crytools.warnmats
	cryTools.warnmats = cryTools.inFromINI "CryTools" "warnmats"
	if cryTools.warnmats == "" then (cryTools.outToINI "CryTools" "warnmats" "true" ; cryTools.warnmats = true)
	else if cryTools.warnmats == "true" then cryTools.warnmats = true else cryTools.warnmats = false
		
		--get crytools.reparenttwist
	cryTools.reparenttwist = cryTools.inFromINI "CryTools" "reparent"
	if cryTools.reparenttwist == "" then (cryTools.outToINI "CryTools" "reparent" "true" ; cryTools.reparenttwist = true)
	else if cryTools.reparenttwist == "true" then cryTools.reparenttwist = true else cryTools.reparenttwist = false
		
		--get crytools.suppresswarnings
	cryTools.suppresswarnings = cryTools.inFromINI "CryTools" "suppress"
	if cryTools.suppresswarnings == "" then (cryTools.outToINI "CryTools" "suppress" "false" ; cryTools.suppresswarnings = false)
	else if cryTools.suppresswarnings == "true" then cryTools.suppresswarnings = true else cryTools.suppresswarnings = false
		
		--get unParentW
	cryTools.nounparentw = cryTools.inFromINI "CryTools" "no_unparent_weapons"
	if cryTools.nounparentw == "" then (cryTools.outToINI "CryTools" "no_unparent_weapons" "true" ; cryTools.nounparentw = true)
	else if cryTools.nounparentw == "true" then cryTools.nounparentw = true else cryTools.nounparentw = false
		
		--get crytools.showSplash
	cryTools.showSplash = cryTools.inFromINI "CryTools" "splash"
	if cryTools.showSplash == "" then (cryTools.outToINI "CryTools" "splash" "true" ; cryTools.showSplash = true)
	else if cryTools.showSplash == "true" then cryTools.showSplash = true else cryTools.showSplash = false
		
		--get ActivateAnimTools
	cryTools.loadOldAnimTools = cryTools.inFromINI "CryTools" "loadOld_animTools"
	if cryTools.loadOldAnimTools == "" then (cryTools.outToINI "CryTools" "loadOld_animTools" "false" ; cryTools.loadOldAnimTools = false)
	else if cryTools.loadOldAnimTools == "true" then cryTools.loadOldAnimTools = true else cryTools.loadOldAnimTools = false		
	
		--get crytools.checkbeforeexport
	cryTools.checkbeforeexport = cryTools.inFromINI "CryTools" "checkExport"
	if cryTools.checkbeforeexport == "" then (cryTools.outToINI "CryTools" "checkExport" "true" ; cryTools.checkbeforeexport = true)
	else if cryTools.checkbeforeexport == "true" then cryTools.checkbeforeexport = true else cryTools.checkbeforeexport = false
)
setupVars = undefined




cryTools.setupCallbacks = fn setupCallbacks =
(
		--cryinfo callback
	callbacks.removescripts id:#setMacros
	callbacks.addscript #filePostOpen ("macros.run \"CryTools\" \"CryInfoLoader\"") id:#setMacros
		--sliderMan callback
	callbacks.removescripts id:#sliderManCall
	callbacks.addScript #selectionSetChanged "crytools.sliderMan()" id:#sliderManCall
			--update menu
	callbacks.removescripts id:#updateMenu
	callbacks.addscript #preSystemShutdown ("try (menuMan.unRegisterMenu (menuMan.findMenu \"CryTools\") ) catch()") id:#updateMenu
	
	
)
setupCallbacks = undefined




cryTools.setupPlugins = fn setupPlugins =
(
	cryTools.maxVersionNum = (maxVersion())[1] / 1000
	print ("Loading CryTools for Max" + cryTools.maxVersionNum as String)
	
	-- See if cryExport is in the plugins folder	
	case crytools.maxVersionNum of
	(
		7: if doesfileexist (crytools.maxDirTxt + "plugins\\CryExport7.dlu") == true then ( crytools.cryExportPresent = true )
		8: if doesfileexist (crytools.maxDirTxt + "plugins\\CryExport8.dlu") == true then ( crytools.cryExportPresent = true )
		9:	if doesfileexist (crytools.maxDirTxt + "plugins\\CryExport9.dlu") == true or doesfileexist (crytools.maxDirTxt + "plugins\\CryExport9_64.dlu") then ( crytools.cryExportPresent = true )
		10: if doesfileexist (crytools.maxDirTxt + "plugins\\CryExport10.dlu") == true or doesfileexist (crytools.maxDirTxt + "plugins\\CryExport10_64.dlu") then ( crytools.cryExportPresent = true )
	)
	
	
	-------------------------------------------------------------------------------
	--update the loader
	-------------------------------------------------------------------------------	
	if doesfileexist (crytools.BuildPathFull + "Tools\\maxscript\\LoadCryTools.ms") != false then
	(
		newLoader = (crytools.md5 (crytools.BuildPathFull + "Tools\\maxscript\\LoadCryTools.ms"))
		oldLoader = (crytools.md5 (crytools.maxDirTxt + "scripts\\startup\\LoadCryTools.ms"))
		if newLoader != oldLoader then
		(
			crytools.minusR (getDir #maxroot + "Scripts\\Startup\\LoadCryTools.ms")
			if (queryBox "Different CryLoader found in the Build.\n\nDo you want to update the Loader?" title:"CryTools") == true then
			(
				cryTools.scmd ("move /Y " + ("\"" + crytools.maxDirTxt + "sctipts\\startup\\LoadCryTools.ms\"") + " " + ("\"" + sysInfo.tempDir + "cry_temp\\\"")) true
				cryTools.scmd ("copy /Y " + ("\"" + crytools.BuildPathFull + "Tools\\maxscript\\LoadCryTools.ms\"") + " " + ("\"" + crytools.maxDirTxt + "scripts\\startup\\\"")) true
				--messagebox "The cryTools loader has been updated, cryTools will now reload with the updated loader."
				print "Updated cryTools loader."
				
				return true
			)
			--return undefined
		)
	)
	
	
	if crytools.maxVersionNum == 7 then
	(
		messageBox "Max Version is unsupported for CryTools." title:"Error loading CryTools"
		return undefined
	)

	--	Check/Install CryExport8.dlu using the MD5 hash
	if crytools.maxVersionNum == 8 then
	(
		if doesfileexist (crytools.maxDirTxt + "plugins\\CryExport7.dlu") == true then
		(
			messageBox "You have CryExport7.dlu installed when you are running Max 8.\nPlease remove CryExport7 and reinstall cryTools or drop CryExport8 in it's place."
			return undefined
		)
		
		-- check for location of the exporter
		if crytools.existfile (crytools.maxDirTxt + "plugins\\CryExport8.dlu") == false then
		(
			crytools.scmd ("move /Y " + ("\"" + crytools.BuildPathFull + "Tools\\CryExport8.dlu\"") + " " + ("\"" + crytools.maxDirTxt + "plugins\\\"")) true
			if crytools.existfile (crytools.maxDirTxt + "plugins\\CryExport8.dlu") == false then
			(
				messageBox "I cannot find your CryExport8.dlu.\nIt isn't in your build directory, and it isn't in your plugins folder; what have you done with it?" title: "Error!"
				return undefined
			)
		)
		
		if doesfileexist (crytools.BuildPathFull + "Tools\\CryExport8.dlu") == false then
		(
			print ((crytools.BuildPathFull + "Tools\\CryExport8.dlu") + " does not exist!")
		)
		else
		(
			--check for +r on old cryExport
			if doesfileexist (sysInfo.tempDir + "cry_temp\\cryExport8.dlu") == true then
			(
				cryTools.scmd ("attrib -r \"" + sysInfo.tempDir + "cry_temp\\cryExport8.dlu\"") true
			)
			newCryExport = (crytools.md5 (crytools.BuildPathFull + "Tools\\CryExport8.dlu"))
			if crytools.md5 (crytools.maxDirTxt + "plugins\\CryExport8.dlu") != newCryExport  then
			(
				if crytools.rollback_status == "false" then
				(
					if (queryBox "A new version of CryExport8.dlu is available, do you want to synch it?" title:"Update CryExport") == true then
					(
						print ("Installing latest CryExport plugin to " + (crytools.maxDirTxt + "plugins\\CryExport8.dlu"))
						scmd ("move /Y " + ("\"" + crytools.maxDirTxt + "plugins\\CryExport8.dlu\"") + " " + ("\"" + sysInfo.tempDir + "cry_temp\\\""))true
						scmd ("move /Y " + ("\"" + crytools.BuildPathFull + "Tools\\CryExport8.dlu\"") + " " + ("\"" + crytools.maxDirTxt + "plugins\\\"")) true
						output_rollbackINI = createfile (sysInfo.tempDir + "\\cry_temp\\crytools.rollback_status.ini")
						format crytools.rollback_status to: output_rollbackINI
						close output_rollbackINI
						print ("Installed newer version of CryExport (" + (newCryExport as string) + ") plugin to " + (crytools.maxDirTxt + "plugins\\" + cryExportName) + " please restart save your work and restart Max.")
					)
				)
			)
		)
	)
	
	--	Check/Install CryExport9.dlu using the MD5 hash
	if crytools.maxVersionNum == 9 then
	(
		if doesfileexist (crytools.maxDirTxt + "plugins\\CryExport8.dlu") == true then
		(
			messageBox "You have CryExport8.dlu installed when you are running Max 9.\nPlease remove CryExport8 and reinstall cryTools or drop CryExport9 in it's place."
			return undefined
		)
		
		cryExportName = "CryExport9.dlu"
		-- check for location of the exporter
		if is64bitApplication != undefined then
		(
			if is64bitApplication() == true then
				cryExportName = "CryExport9_64.dlu"
		)
		
			
		if crytools.existfile (crytools.maxDirTxt + "plugins\\" + cryExportName) == false then
		(
			cryTools.scmd ("move /Y " + ("\"" + crytools.BuildPathFull + "Tools\\" + cryExportName + "\"") + " " + ("\"" + crytools.maxDirTxt + "plugins\\\"")) true
			if crytools.existfile (crytools.maxDirTxt + "plugins\\" + cryExportName) == false then
			(
				--messageBox "I cannot find your CryExport9.dlu.\nIt isn't in your build directory, and it isn't in your plugins folder; what have you done with it?" title: "Error!"
				print ("Cannot find " + cryExportName)
				return undefined
			)
		)
	
		if doesfileexist (crytools.BuildPathFull + "Tools\\" + cryExportName) == false then
		(
			print ((crytools.BuildPathFull + "Tools\\" + cryExportName) + " does not exist!")
			--return undefined
		)
		else
		(
			--check for +r on old cryExport
			if doesfileexist (sysInfo.tempDir + "cry_temp\\" + cryExportName) == true then
			(
				cryTools.scmd ("attrib -r \"" + sysInfo.tempDir + "cry_temp\\" + cryExportName + "\"") true
			)
			newCryExport = (crytools.md5 (crytools.BuildPathFull + "Tools\\" + cryExportName))
			if crytools.md5 (crytools.maxDirTxt + "plugins\\" + cryExportName) != newCryExport  then
			(
				if crytools.rollback_status == "false" then
				(
					print ("Installing latest CryExport plugin to " + (crytools.maxDirTxt + "plugins\\" + cryExportName))
					cryTools.scmd ("move /Y " + ("\"" + crytools.maxDirTxt + "plugins\\" + cryExportName +"\"") + " " + ("\"" + sysInfo.tempDir + "cry_temp\\\"")) true
					cryTools.scmd ("move /Y " + ("\"" + crytools.BuildPathFull + "Tools\\" + cryExportName + "\"") + " " + ("\"" + crytools.maxDirTxt + "plugins\\\"")) true
					output_rollbackINI = createfile (sysInfo.tempDir + "\\cry_temp\\crytools.rollback_status.ini")
					format crytools.rollback_status to: output_rollbackINI
					close output_rollbackINI
				)
				--messageBox ("Installed newer version of CryExport (" + (newCryExport as string) + ") plugin to " + (crytools.maxDirTxt + "plugins\\" + cryExportName) + " please restart save your work and restart Max.") title: ((crytools.maxDirTxt + "plugins\\" + cryExportName) + " installed!")
				print ("Installed newer version of CryExport (" + (newCryExport as string) + ") plugin to " + (crytools.maxDirTxt + "plugins\\" + cryExportName) + " please restart save your work and restart Max.")
			)
		)
	)
	
	
	
	--	Check/Install CryExport9.dlu using the MD5 hash
	if crytools.maxVersionNum == 10 then
	(
		if doesfileexist (crytools.maxDirTxt + "plugins\\CryExport9.dlu") == true then
		(
			messageBox "You have CryExport9.dlu installed when you are running Max 10.\nPlease remove CryExport9 and reinstall cryTools or drop CryExport10 in it's place."
			return undefined
		)
		
		cryExportName = "CryExport10.dlu"
		-- check for location of the exporter
		if is64bitApplication != undefined then
		(
			if is64bitApplication() == true then
				cryExportName = "CryExport10_64.dlu"
		)
		
			
		if crytools.existfile (crytools.maxDirTxt + "plugins\\" + cryExportName) == false then
		(
			cryTools.scmd ("move /Y " + ("\"" + crytools.BuildPathFull + "Tools\\" + cryExportName + "\"") + " " + ("\"" + crytools.maxDirTxt + "plugins\\\"")) true
			if crytools.existfile (crytools.maxDirTxt + "plugins\\" + cryExportName) == false then
			(
				--messageBox "I cannot find your CryExport9.dlu.\nIt isn't in your build directory, and it isn't in your plugins folder; what have you done with it?" title: "Error!"
				print ("Cannot find " + cryExportName)
				return undefined
			)
		)
	
		if doesfileexist (crytools.BuildPathFull + "Tools\\" + cryExportName) == false then
		(
			print ((crytools.BuildPathFull + "Tools\\" + cryExportName) + " does not exist!")
			--return undefined
		)
		else
		(
			--check for +r on old cryExport
			if doesfileexist (sysInfo.tempDir + "cry_temp\\" + cryExportName) == true then
			(
				cryTools.scmd ("attrib -r \"" + sysInfo.tempDir + "cry_temp\\" + cryExportName + "\"") true
			)
			newCryExport = (crytools.md5 (crytools.BuildPathFull + "Tools\\" + cryExportName))
			if crytools.md5 (crytools.maxDirTxt + "plugins\\" + cryExportName) != newCryExport  then
			(
				if crytools.rollback_status == "false" then
				(
					print ("Installing latest CryExport plugin to " + (crytools.maxDirTxt + "plugins\\" + cryExportName))
					cryTools.scmd ("move /Y " + ("\"" + crytools.maxDirTxt + "plugins\\" + cryExportName +"\"") + " " + ("\"" + sysInfo.tempDir+ "cry_temp\\\"")) true
					cryTools.scmd ("move /Y " + ("\"" + crytools.BuildPathFull + "Tools\\" + cryExportName + "\"") + " " + ("\"" + crytools.maxDirTxt + "plugins\\\"")) true
					output_rollbackINI = createfile (sysInfo.tempDir + "\\cry_temp\\crytools.rollback_status.ini")
					format crytools.rollback_status to: output_rollbackINI
					close output_rollbackINI
				)
				--messageBox ("Installed newer version of CryExport (" + (newCryExport as string) + ") plugin to " + (crytools.maxDirTxt + "plugins\\CryExport9.dlu") + " please restart save your work and restart Max.") title: ((crytools.maxDirTxt + "plugins\\" + cryExportName) + " installed!")
				print ("Installed newer version of CryExport (" + (newCryExport as string) + ") plugin to " + (crytools.maxDirTxt + "plugins\\" + cryExportName) + " please restart save your work and restart Max.")
			)
		)
	)
	
	
	
	-- Check/Install AutoDesk Physique Maxscript Exposure
	if is64bitApplication == undefined then	
	(
		if getFileSize (crytools.maxDirTxt + "plugins\\IPhysique.gup") != 25088 then
		(
			--messageBox ("Installing latest AutoDesk Physique Exposure plugin to "+ (crytools.maxDirTxt + "plugins\\IPhysique.gup") + " please restart save your work and restart Max.") title: ((crytools.maxDirTxt + "plugins\\IPhysique.gup") + " installed!")
			print ("Installing latest AutoDesk Physique Exposure plugin to "+ (crytools.maxDirTxt + "plugins\\IPhysique.gup") + " please restart save your work and restart Max.")
			cryTools.scmd ("copy /Y \"" + (crytools.BuildPathFull + "Tools\\maxscript\\autoinstall\\IPhysique.gup\"") + " " + ("\"" + crytools.maxDirTxt + "plugins\\\"")) true
		)
		else
		(
			print "no new cryexport"
		)
	)
	
	
	return false
)
setupPlugins = undefined




cryTools.setupTools = fn setupTools =
(
	cryTools.setupVars()
	

	if perforcePath_crytools != undefined then
	(
		if doesfileexist (BuildPathFull_crytools + "tools\\maxscript\\md5.exe") == false then
		(
			cryTools.scmd ("p4 sync \"" + BuildPathFull_crytools + "Tools\\...\"") true
	
			print "syncing p4 to get md5.exe"
		)
	)


	if crytools.showSplash == true then
	(
		global crysplash
		rollout crySplash ""
		(
			label version "Version 2.0" pos:[9,108]
			label build "" pos:[160,108] text:"TOOLS LOADING"
		
			on crySplash lbuttonup position do (destroyDialog crysplash)
		)			
		bm = (openBitmap (crytools.BuildPathFull + "Tools\\maxscript\\icons\\splash.png"))
		createDialog crysplash style:#(#style_border) modal:false bitmap:bm width:bm.width height:bm.height fgcolor:white			
		
	)
	
	

	if crytools.showSplash == true then
		crysplash.build.text = "ENV CHECK"
	
	
	global loadingResult = cryTools.setupPlugins()
	if loadingResult == undefined or loadingResult == true then
		return undefined
	
	cryTools.setupCallbacks()
	cryTools.generateMenu()
	cryTools.setupMacros()
	
	if cryTools.loadOldAnimTools == false then
	(
		try
		(			
			filein (crytools.BuildPathFull + "Tools\\maxscript\\cryAnim\\load.ms")
		)
		catch ( print "Error loading cryAnim Tools." )
	)
	
	
	
	
	
	if crytools.showSplash == true then
	(
		crysplash.build.text = ("BUILD: " + (crytools.localBuildNumber as string) + "/" + (crytools.latestbuildnumber as string))
		sleep .5
		destroydialog crysplash
	)
)
setupTools = undefined


-----------------------------------------------------------------------------------------
--------------------------CHECKING SCENE FOR EXPORT--------------------------
-----------------------------------------------------------------------------------------



crytools.CheckObjMat = fn CheckObjMat =
(
	--crytools.ObjMatOk = true
	for obj in csexport.export.get_node_list() do
	(
		if crytools.warnmats == true then
		(
			if obj.children.count != 0 then
			(
				for obj1 in (crytools.getChildren obj) do
				(
					if obj1.material == undefined then
					(
						if (classof obj1) != Dummy then
						(
							 append crytools.PreExportWarnings ("There is no defined shader for this object!  " + obj1.name)
						)
					)
				)
			)
			else
			(
				if obj.material == undefined then
				(
					if (classof obj) != Dummy then
					(
						append crytools.PreExportWarnings ("There is no defined shader for this object!  " + obj.name)
					)
				)
			)

			try
			(
				if crytools.texturesOK == true then
				(
					if obj.children.count != 0 then
					(
						for obj1 in (crytools.getChildren obj) do
						(
							if (classof obj1) != Dummy then
							(
								if obj1.material != undefined then
								(
									materials = obj1.material.materialIDlist
									for i = 1 to materials.count do
									(
										checkme = obj1.material.material[i].shadername
										if checkme != "Crytek Shader" then
										(
											append crytools.PreExportWarnings ("The following material is not set to the Crytek Shader: " + obj1.material.material[i].name)
										)
									)
								)
							)
						)
					)
					else
					(
						if (classof obj) != Dummy then
						(
							if obj.material != undefined then
							(
								materials = obj.material.materialIDlist
								for i = 1 to materials.count do
								(
									checkme = obj.material.material[i].shadername
									if checkme != "Crytek Shader" then
									(
										append crytools.PreExportWarnings ("The following material is not set to the Crytek Shader: " + obj.material.material[i].name)
									)
								)
							)
						)
					)
				)
			)
			catch
			(
				if (classof obj) != Dummy then
				(
					if obj.material != undefined then
					(
						if (classof obj.material) == MultiMaterial then
						(
							materials = obj.material.materialList
							if materials.count > 0 then
							(
							 	if materials.count > 32 then
								(
							 		append crytools.PreExportWarnings ("material '" + obj.material.name + " has '" + materials.count as string + "' number of submaterials, you may wish to clean the multimaterial using max, its on the utilities menu in the material editor")
								)

								for i = 1 to materials.count do
								(
							 		if i <= 32 then
							 		(
										checkme = obj.material.material[i].shadername
										if checkme != "Crytek Shader" then
										(
											append crytools.PreExportWarnings ("The following material is not set to the Crytek Shader: " + obj.material.material[i].name)
										)
									)
								)
							)
						)
						else
						(
							checkme = obj.material.shadername
							if checkme != "Crytek Shader" then
							(
								append crytools.PreExportWarnings ("The following material is not set to the Crytek Shader: " + obj.material.name)
							)
						)		
					)
				)
			)
		)
	)
)

crytools.ReparentTwistBns = fn ReparentTwistBns =
(
	if $'Bip01 R ForeTwist' != undefined then
	(
		if $'Bip01 R ForeTwist'.parent.name != "Bip01 R Forearm" then
		(
			$'Bip01 R ForeTwist'.parent = $'Bip01 R Forearm'
			if crytools.suppresswarnings == true then
			(
				print "Biped lower arm twist bones reparented."
			)
			else
			(
				messageBox "Biped lower arm twist bones reparented."
			)
		)
		if $'Bip01 L ForeTwist'.parent.name != "Bip01 L Forearm" then
		(
			$'Bip01 L ForeTwist'.parent = $'Bip01 L Forearm'
		)
	)
	
	if $'Bip01 LUpArmTwist' != undefined then
	(
		if $'Bip01 LUpArmTwist'.parent.name != "Bip01 L UpperArm" then
		(
			$'Bip01 LUpArmTwist'.parent = $'Bip01 L UpperArm'
			if crytools.suppresswarnings == true then
			(
				print "Biped upper arm twist bones reparented."
			)
			else
			(
				messageBox "Biped upper arm twist bones reparented."
			)
		)
		if $'Bip01 RUpArmTwist'.parent.name != "Bip01 R UpperArm" then
		(
			$'Bip01 RUpArmTwist'.parent = $'Bip01 R UpperArm'
		)
	)
)

crytools.WpnBnUnlink = fn WpnBnUnlink =
(
	if $weapon_bone != undefined then
	(
		if $weapon_bone.children.count != 0 then
		(
			undo "unlinking $weapon_bone children" on
			(
				for obj in $weapon_bone.children do
				(
				obj.parent = undefined
					if crytools.suppresswarnings == true then
					(
						print "unlinking $weapon_bone children"
					)
					else
					(
						messagebox "unlinking $weapon_bone children"
					)
				)
			)
		)
	)
)

crytools.CheckNodeTm = fn CheckNodeTm =
(
	for obj in csexport.export.get_node_list() do
	(
		---check rotation---
		t = obj.rotation
		if (t.x*t.x + t.y*t.y + t.z*t.z) > 0.0001 then
		(
			append crytools.PreExportWarnings ("The object '" + obj.name + "' has a rotation applied to it " + ((obj.rotation as eulerangles) as string) + "\nPlease Reset Xform the object before exporting.")
		)
		---check scale---
		t = obj.scale - [1,1,1]
		if (t.x*t.x + t.y*t.y + t.z*t.z) > 0.0001 then
		(
			append crytools.PreExportWarnings ("The object '" + obj.name + "' has an abnormal scale " + ((obj.scale) as string) + "\nPlease Reset Xform the object before exporting.\nScale should be [1,1,1]")
		)
	)
)

crytools.CheckNonZeroMorphs = fn CheckNonZeroMorphs =
(
	for obj in csexport.export.get_node_list() do
	(
		try
		(	
			for i = 1 to 1000 do
			(
				test = obj.morpher[i].name
				if test == "- empty -" then
				(
					exit loop
				)
				if obj.morpher[i].value != 0.0 then
				(
					append crytools.PreExportWarnings (test + " has a non zero value! " + "(" + (obj.morpher[i].value as string) + ")")
				)
			)
		)
		catch()
	)
)

crytools.CheckBnScale = fn CheckBnScale =
(
	for obj in csexport.export.get_bone_list() do
	(
		try
		(
			if obj.children.count != 0 then
			(
				for obj1 in (crytools.getChildren obj) do
				(
					t = obj1.scale - [1,1,1]
					if (t.x*t.x + t.y*t.y + t.z*t.z) > 0.0001 then
					(
						append crytools.PreExportWarnings (obj1.name + " has a scale applied: " + (obj1.scale as string) + "\nPlease reset it's scale before exporting.")
					)	
				)
			)
			else
			(
				t = obj.scale - [1,1,1]
				if (t.x*t.x + t.y*t.y + t.z*t.z) > 0.0001 then
				(
					append crytools.PreExportWarnings (obj.name + " has a non zero scale (" + (obj.scale as string) + ")\nPlease reset it's scale before exporting.")
				)
			)
		)
		catch()
	)
)

crytools.CheckVerts = fn CheckVerts =
(
	for obj in csexport.export.get_node_list() do
	(
		try
		(
			ClsVrts = #()
			VertsArr = #()
			for x in obj.verts do
			(
				Vrts = #(x.index, x.pos.x, x.pos.y, x.pos.z)
				append VertsArr Vrts
			)
			
			fn CompareFN v1 v2 =
			(
				local d = (length [v1[2],v1[3],v1[4]]) - (length [v2[2],v2[3],v2[4]])
				case of
				(
					(d < 0) : -1
					(d > 0) : 1
					default: 0
				)
			)
			
			qsort VertsArr CompareFN
			
			for x = 1 to VertsArr.count do
			(
				if VertsArr[x+1] != undefined then
				(
					DiffX = (VertsArr[x+1][2] - VertsArr[x][2])
					if DiffX == 0 then DiffX = 0.001
					if DiffX < 0.08 and DiffX > -0.08 then
					(
						DiffY = (VertsArr[x+1][3] - VertsArr[x][3])
						if DiffY == 0 then DiffY = 0.001
						if DiffY < 0.08 and DiffY > -0.08 then
						(
							DiffZ = (VertsArr[x+1][4] - VertsArr[x][4])
							if DiffZ == 0 then DiffZ = 0.001
							if DiffZ < 0.08 and DiffZ > -0.08 then
							(
								if (distance [VertsArr[x+1][2],VertsArr[x+1][3],VertsArr[x+1][4]] [VertsArr[x][2],VertsArr[x][3],VertsArr[x][4]]) < 0.08 then
								(
									Tmp = #()
									append Tmp VertsArr[x+1][1]
									append Tmp VertsArr[x][1]
									append ClsVrts Tmp
								)
							)
						)
					)
				)
			)
			if ClsVrts.count != 0 then
			(
				SelVerts = #{}
				Txt = ""
				for x in ClsVrts do
				(
					if (FindItem ClsVrts  x) == ClsVrts.count then
					(
						Txt += (x as string) + ". "
					)
					else
					(
						Txt += (x as string) + ", "
					)
					for num in x do
					(
						append SelVerts num
					)
				)
				append crytools.PreExportWarnings (obj.name + " has these verts too close to each other: " + Txt + "Please move them further appart.")
				if ($.baseobject as string) == "Editable Poly" then
				(
					polyop.setVertSelection obj SelVerts
				)
				else
				(
					SetVertSelection obj SelVerts
				)
			)
		)
		catch (print "Object is not a mesh/poly!")
	)
)

-- check scene for export
crytools.CheckSceneForExport = fn CheckSceneForExport =
(
	--try
	--(
		if crytools.checkbeforeexport == false then return true
		
		crytools.texturesOK = true
		crytools.PreExportWarnings = #()
		----------------------Material Check-----------------------------------------------
		crytools.CheckObjMat ()
		----------------------Vertex Check-------------------------------------------------
		crytools.CheckVerts ()
		-------------------------Reparent---------------------------------------------------
		if crytools.reparenttwist == true then ReparentTwistBns ()
		------------------Weapon Bone Unlinking-------------------------------------------
		-- check for children of the weapon bone
		if crytools.nounparentw == false then WpnBnUnlink ()
		-----------------check for non zero morphs-----------------------------------------
		crytools.CheckNonZeroMorphs ()
		---------------check if reset transform needed------------------------------------
		crytools.CheckNodeTm ()
		----------------------check bone scale----------------------------------------------
		crytools.CheckBnScale ()
		--------------------------------------------------------------------------------------
		
		if crytools.PreExportWarnings.count != 0 then crytools.texturesOK = false
			
		if crytools.suppresswarnings == false then
		(
			strng = "-------------------ERROR----------------\n"
			For x = 1 to (crytools.PreExportWarnings.count-1) do
			(
				strng = strng + crytools.PreExportWarnings[x] as string +"\n\n-------------------ERROR----------------\n"
			)
			
			if cryTools.PreExportWarnings.count > 0 then
				messagebox (Strng + crytools.PreExportWarnings[crytools.PreExportWarnings.count] as string) title:"Problem Found!"
		)
		
		if crytools.texturesOK == false then
		(
			return false
			--print "bad"
		)
		else
		(
			return true
			--print "good"
		)
	--)
	--catch (undefined)	
)



-- register export callback
csexport.export.register_export_callback crytools.CheckSceneForExport






cryTools.setupTools()

if loadingResult == true then
(
	try ( destroyDialog crysplash ) catch()
	filein (crytools.maxDirTxt + "scripts\\startup\\LoadCryTools.ms")
	print "Updated CryLoader"
)

loadingResult = undefined
