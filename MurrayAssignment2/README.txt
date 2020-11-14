In order to make building on Windows relatively easier, I compiled the systemc library and used cmake to build and link my code. 

To build yourself, you will need to change the CMakeLists.txt file lines 7 and 8. 
Here, specify the directories that contain the systemc header files and the library respectively. 

Since I used the MSVC compiler, I also had to set the /vmg and /GR compiler options on line 9, these will need to be removed on Linux.

I also used vscode with the cmake extension to automate configuring and building. 

To use the command line you should be able to do the following:

cmake -S . -B build

Then on Windows using MSVC:
cmake --build build --config Release --target ALL_BUILD

Or on Linux (tested with WSL Ubuntu 20.04):
cmake --build build
