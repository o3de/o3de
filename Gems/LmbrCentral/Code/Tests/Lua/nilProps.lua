local nilProps =
{
    Properties =
    {
        myVar = "ABCD",
    }
}

function nilProps:OnActivate()
    Debug.Log(tostring(self.Properties.myVar));
end

return nilProps