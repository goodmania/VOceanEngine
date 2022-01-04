# VOceanEngine

 VOceanEngine is an FFT ocean simulation that uses Vulkan and C++.  
   
# DEMO
 
You can generate oceans like the following image.
 
![FFTOcean](https://user-images.githubusercontent.com/63941325/147940555-e55b60e1-26a8-4b9b-aa76-d399b58bde6e.gif)
  
# Requirement
 
* C++17
* vulkan SDK
* Windows environment
* visual stdio 2019

# Installation
 
1. Install VOceanEngine with git clone command.
 
```bash
git clone --recursive https://github.com/goodmania/VOceanEngine.git
```

2. Install the latest version of [Vulkan SDK.](https://vulkan.lunarg.com/sdk/home)

3. Save the vulkanSDK folder in the following location.

```bash
VOceanEngine\vender
```

4. Run "GenerateProject.bat"

# Usage
 
* `W`, `A`, `S`, `D` horizontal movement.
* `E`, `F` Vertical movement.
* `Mouse Movement` Camera rotation.
 
# Note
 
* This application will only work in a Windows environment.

# Reference
* Engine
  * [Hazel](https://github.com/TheCherno/Hazel)
  * [littleVulkanEngine](https://github.com/blurrypiano/littleVulkanEngine)
  * [acid](https://github.com/EQMG/Acid/tree/master/Sources)
  * [SaschaWillems/Vulkan](https://github.com/SaschaWillems/Vulkan)

* FFT Ocean Theory
  * [Jerry Tessendorf(2001). Simulating Ocean Water](https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.161.9102&rep=rep1&type=pdf)
  * [[Unity] 海洋シミュレーションFFT Oceanを実装したい](https://qiita.com/Red_Black_GPGPU/items/2652f5bfd6d311d2034b) 
 
# Author
 
* goodmania
 
# License
 
"VOceanEngine" is under [MIT license](https://en.wikipedia.org/wiki/MIT_License).
 
