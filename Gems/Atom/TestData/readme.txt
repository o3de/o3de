Atom test projects should include <Atom GemRoot>\TestData in their Asset Processor "ScanFolder" list, like this:

[ScanFolder AtomTestData]
watch=@GEMROOT:Atom@/TestData
recursive=1
order=1000

The reason we have a second "TestData" folder inside this one is to make it very clear that the corresponding assets in the cache are from the TestData folder. For example, the asset path at runtime will be like "testdata/objects/cube.azmodel" instead of "objects/cube.azmodel".
