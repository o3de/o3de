-------------------------Limited Edge Loop---------------------------
if CryModelling == undefined then
(
	FileIn (BuildPathFull_crytools + "Tools\\maxscript\\CryModelling.ms")
)

case subobjectLevel of
(
	1: CryModelling.SelVertLoop()
	2: CryModelling.SelLimELoop ()
	4: CryModelling.SelPolyLoop()
)
