Linux build instructions
------------------------

Useful:
  readelf -d ./syntaxic

Windows build instructions
--------------------------

 Various facts about Windows builds:
  - there are 2 types of builds:
     * multithreaded dynamic (MD)
	 * multithreaded (MT)
	It is important that all artifacts are compiled in the same way.
	TDE uses MD artifacts which are more common.  Care must be taken
	to find those for 3rd party libraries.

	No debug artifacts are used.
  - Linking to a DLL requires a linking library which is a .lib

 Steps:
  - Run CMake GUI first.
  - Open solution file in VS2013
  - In configuration manager, switch to Release builds for tde.exe and utests.exe
  - Build

 Notes:

 - I don't know why the boost libraries are titled _mt, when they are evidently
 MD.
 - Use dependancy walker to figure out DLL requirements