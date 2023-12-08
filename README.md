# MakeOBJ
MakeOBJ converts files under DOS into the OMF format, which can then be linked directly into your program.
Files can also be converted back from OMF format to the original format.
Furthermore, files can be converted into char arrays.

## Build
The project should be compileable with any Borland C++ version from 2.0 up to and including 3.1 under DOS.
To build MakeOBJ you can use the batch file make.bat in the root directory of the repository. 
MakeOBJ should be built with support for wildcard characters and should therefore be linked with wildargs.obj. 
If necessary, you may have to adjust the location of the wildargs.obj in the make.bat to match your Borland C++ installation.

## License
The license can be seen [here](https://github.com/skynettx/makeobj/blob/master/LICENSE).

## Thanks
This project is based on the makeobj.c of the [keen](https://github.com/keendreams/keen) project, my thanks to everyone involved.









