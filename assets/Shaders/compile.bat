C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag testFragment.glsl -o testFragment.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex testVertex.glsl -o testVertex.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag ComputeTest/particleFragment.glsl -o ComputeTest/particleFragment.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex ComputeTest/particleVertex.glsl -o ComputeTest/particleVertex.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=compute ComputeTest/computeTest.glsl -o ComputeTest/computeTest.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag HeightmapTest/heightmapFragment.glsl -o HeightmapTest/heightmapFragment.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex HeightmapTest/heightmapVertex.glsl -o HeightmapTest/heightmapVertex.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag gltfTestFrag.glsl -o gltfTestFrag.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex gltfTestVert.glsl -o gltfTestVert.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag ThreadingTest/threadingFrag.glsl -o ThreadingTest/threadingFrag.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex ThreadingTest/threadingVert.glsl -o ThreadingTest/threadingVert.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag CubemapTest/skyboxFrag.glsl -o CubemapTest/skyboxFrag.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex CubemapTest/skyboxVert.glsl -o CubemapTest/skyboxVert.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=frag CubemapTest/reflectFrag.glsl -o CubemapTest/reflectFrag.spv
C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=vertex CubemapTest/reflectVert.glsl -o CubemapTest/reflectVert.spv

C:/VulkanSDK/1.1.130.0/Bin32/glslc.exe -fshader-stage=compute ComputeTransferTest/compTransferComp.glsl -o ComputeTransferTest/compTransferComp.spv
pause