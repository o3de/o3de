mkdir DX12
cd DX12
cmake ..\.. -DGFX_API=DX12
cd ..

mkdir VK
cd VK
cmake ..\.. -DGFX_API=VK
cd ..