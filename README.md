# Yarn Desktop Client

A desktop client for Yarn ( https://yarn.social/ ).  
It's being developed and tested on Debian, and has flakes file NixOS.  

# Nixos:  
Flake:  
Enable flake support, then go to source dir and type ```nix build```.  

Nix-shell:  
Go to source dir, type ```nix-shell```.  
Then:  
```mkdir build && cd build && cmake ../ && make```  

# Debian/Ubuntu based:  
Install compiler and tools:  
```sudo apt-get install build-essential cmake git```

Install Yarn desktop client dependencies with:  
```sudo apt-get install libcurl4-openssl-dev libgtk-4-dev rapidjson-dev libsecret-1-dev```  

Or compile the dependencies from source and install that if needed.  

Then clone this repository, and cd into it.  
```git clone https://github.com/stig-atle/YarnDesktopClient/```  
Create a build folder (```mkdir build```) and cd into that.  
Then call cmake with:  

```cmake -DCMAKE_BUILD_TYPE=Debug ../```  
or:  
```cmake -DCMAKE_BUILD_TYPE=Release ../```  
from build directory, then run make to build.  

chmod+x the executable and run it.  

![image](https://github.com/stig-atle/YarnDesktopClient/assets/7627610/7663c80d-3518-4050-bd26-c719d9d0fe0e)

You can reach out to me through the github issue system, or yarn:  
https://yarn.stigatle.no/user/stigatle/

Precompiled packages for various systems and snap \ appimage are planned for later.
