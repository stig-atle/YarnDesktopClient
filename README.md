# Yarn Desktop Client

A desktop client for Yarn, developed and tested on Trisquel (same as Ubuntu\Debian).  
Install compiler and tools:  
```sudo apt-get install build-essential cmake git```

Install Yarn desktop client dependencies with:  
```sudo apt-get install liburl-dev libgtk-4-dev rapidjson-dev```

Or compile the dependencies from source and install that if needed.  

Then clone this repository, and cd into it.  
```git clone https://github.com/stig-atle/YarnDesktopClient/```  
Create a build folder (```mkdir build```) and cd into that.  
Then call cmake with:  

```cmake ../```  
from build directory, then run make to build.  

chmod+x the executable and run it.  

You can reach out to me through the github issue system, or yarn:  
https://yarn.stigatle.no/user/stigatle/

Precompiled packages for various systems and snap \ appimage are planned for later.
