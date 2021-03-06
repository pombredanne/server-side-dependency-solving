server-side-dependency-solving - SSDS
==============================

Description
===========
Solving dependencies when installing new packages is a process that in some cases can be computationally very intensive. SSDS is designed to overcome this issue. Client device gathers information about installed packages and enabled repos which are then sent to a server. The server then takes care of dependency solving and the result is sent back to client device as a list of packages that are needed to proceed with the install process and without dependency solving on client.

Dependencies
============
Required packages needed for successfull compilation and build.

- gcc
- gcc-c++
- cmake
- libsolv
- librepo-devel
- hawkey-devel
- glib2-devel
- json-glib
- json-glib-devel
- check-devel

Installation
============
This section describes how to install and compile SSDS project

There is new module in librepo, but it is not among updates yet so this is what needs to be done to be able to use it:
  -from http://koji.fedoraproject.org/koji/taskinfo?taskID=8948750 download these:
   librepo, python-librepo, librepo-devel - all in the right architecture
   these three should be ebough but just in case you can download also librepo-debuinfo and python3-librepo
   there is new version with some fixes - 1.7.13-4.fc21.x86_64
  -install these packages at the same time like this:
   $sudo dnf update <pkg.rpm> <pkg.rpm> ... - put them all into the same command

Now everything should be in order to build rds:
  -enter build/ and type:

		$mkdir build

		$cd build/
    
    		$cmake ..
    
    		$make
  
  -two files are created - rds-client and rds-server
	-to test type in build/ dir:

		$ctest
		
	-alternatively (if $ctest runs only one test) you can run test from build/tests/ dir:
	
		$./test_main
  
Enjoy :)

Valgrind
========
If you want to test the program with valgrind, there are lots of warnings and errors that are not caused by this program. Basically these errors are couased by various shared libraries and there is nothing we can do about them. To suppress all these warnings and errors the client program needs to be run with suppression file suppress-cl.supp. The server program has suppress-srv.supp. Both files are located in the build folder. Just run the program with valgrind like this:

	$valgrind --suppressions=suppress-cl.supp ./rds-client ...
	$valgrind --suppressions=suppress-srv.supp ./rds-server ...
