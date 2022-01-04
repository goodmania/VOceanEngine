# VOceanEngine

 VOceanEngine is an FFT ocean simulation that uses Vulkan.  
   
# DEMO
 
You can generate oceans like the following image.
 
![FFTOcean](https://user-images.githubusercontent.com/63941325/147940555-e55b60e1-26a8-4b9b-aa76-d399b58bde6e.gif)
  
# Requirement
 
* c++17
* vulkan SDK
 
Environments under [Anaconda for Windows](https://www.anaconda.com/distribution/) is tested.
 
```bash
conda create -n pyxel pip python=3.6 Anaconda
activate pyxel
```
 
# Installation
 
Install Pyxel with pip command.
 
```bash
pip install pyxel
```
 
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
