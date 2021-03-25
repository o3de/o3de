------Display Vertex Alpha------
for obj in selection do
(
	if obj.showVertexColors == false then
	(
		obj.vertexColorType = #alpha
		obj.showVertexColors = true
	)
	else
	(
		if obj.vertexColorType ==#color then
		(
			obj.vertexColorType = #alpha
			obj.showVertexColors = false
			obj.showVertexColors = true
		)
		else
		(
			obj.showVertexColors = false
		)
	)
)