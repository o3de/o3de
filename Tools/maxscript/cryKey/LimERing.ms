-------------------------Limited Edge Ring---------------------------
if CryModelling == undefined then
(
	FileIn (BuildPathFull_crytools + "Tools\\maxscript\\CryModelling.ms")
)

case subobjectLevel of
(
	1: CryModelling.SelVertRing()
	2: CryModelling.SelLimERing()
)
