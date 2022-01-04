# VOceanEngine

 VOceanEngine is an FFT ocean simulation that uses Vulkan and C++.  
   
# DEMO
 
You can generate oceans like the following image.
 
![FFTOcean](https://user-images.githubusercontent.com/63941325/147940555-e55b60e1-26a8-4b9b-aa76-d399b58bde6e.gif)
  
# Requirement
 
* C++17
* vulkan SDK
* Windows environment  

# Installation
 
1. Install VOceanEngine with git clone command.
 
```bash
git clone --recursive https://github.com/goodmania/VOceanEngine.git
```

2. Install the latest version of [Vulkan SDK.](https://vulkan.lunarg.com/sdk/home)

3.  

# Usage
 
Please create python code named "demo.py".
And copy &amp; paste [Day4 tutorial code](https://cpp-learning.com/pyxel_physical_sim4/).
 
Run "demo.py"
 
```bash
python demo.py
```
 
# Note
 
I don't test environments under Linux and Mac.
 
# Author
 
* Hayabusa
* R&D Center
* Twitter : https://twitter.com/Cpp_Learning
 
# License
 
"Physics_Sim_Py" is under [MIT license](https://en.wikipedia.org/wiki/MIT_License).
 
Enjoy making cute physics simulations!
 
Thank you!
・premake.exeをvender\bin\premakeに保存する。  
・tyni_obj_loaderのリンクを修正する。  
・vulkanSDKをインストールし、VOceanEngine\vender\VulkanSDKにIncludeフォルダ、LIBフォルダをコピーする。  
