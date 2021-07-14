How to Run:

From main.cpp :
Put the test cases in a file named “ti.txt” (i is file number) in folder with name as passed as arg3.
This folder should lie in the same folder in which the main.cpp file is present.
Compile the file with “g++ -o main main.cpp” command. Executable “main.exe” is created.
Run the executable by typing “main.exe arg1 arg2 arg3 arg4 arg5” in command line.
arg1 is N(number of files), arg2 is M(max clock cycles), arg3 is folder name in which test files are present ,arg4 is for row access delay, arg5 is for col access delay
(Wrong command line arguments may generate garbage outputs).
Output is printed in the console. 

From codeblocks project (mipsMemoryRequestManager) :
Have a file “ti.txt” (i is file number) with mips program in a folder name as passed as arg3 which is placed inside the codeblocks project folder.
Set command line arguments as mentioned above by going in Projects->Set program’s arguments (in codeblocks interface). 
Build and run. 
