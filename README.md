Voxel-Engine
============
A CPU based sparse octree renderer.

**Voxel-Engine** is a program that aims to render voxels, stored in a sparse octree, 
interactively using mostly the CPU.

The algorithm works by rendering the cubemap at the location of the camera. 
First it is determined which parts of the cubemap are visible within the viewing pyramid. 
(We have a pyramid instead of a frustum, because we do not have a near and far plane.) 
Then the octree, containing the scene, is rendered to the visible parts of the 6 faces of the cubemap.
The modified faces are then send to OpenGL and rendered as a cubemap.

Dependencies
------------
The **Voxel-Engine** requires a POSIX compliant operating system. (Thus it does not work on Microsoft Windows)
Furthermore it uses the following libraries:

 - GLM: OpenGL Mathematics
 - OpenGL
 - SDL
 - SDL_Image
 
The **Voxel-Engine** itself does not use SDL_Image, but this library is used by some of the
tools accompanying the program.

Compilation
-----------
The program and tools should compile by running `make`.

Execution
---------
After compilation, the **Voxel-Engine** program is executed by:

    ./voxel model

The model argument specifies which `.oct` file in the `vxl/` directory will be loaded. 
The name must be specified without `.oct`, for example: `./voxel sign`.

Tools
-----

    ./build_db pointset [mask repeats]

Converts the given model, stored as `vxl/pointset.vxl` into octree format. 
This process contains a sorting step that reorders the points in the original file.
The output, `vxl/pointset.oct` can be loaded into the renderer by running `./pointset model`. 

The repeat argument can be used to create a model consisting of `2^repeats` copies of the model in the X, Y and Z directions.
The directions in which the model are repeated can be limited using the mask, which is a bitwise -or combination of X=4, Y=2 and Z=1. 
The model will not be copied into the specified directions. 

    ./ascii2bin pointset
    
Converts a `.vxl.txt` file, which is in ASCII format into a `.vxl` file that is in binary format.
The file pointset must reside in `vxl/` and be specified without its extension.
A backup is created of the original file.

    ./cubemap
    
This is a small testing program, which renders a cubemap loaded from `img/cubemap#.png` with `#` ranging from 0 to 5.

    ./convert lidar-ascii-file
    
Used to convert a file in LiDaR ASCII format to a `.vxl.txt` file. 
It skips the first line which is assumed to contain the table header.
This program contains some hard coded numbers which need to be tuned when converting a new file.
Furthermore, this program needs to be updated to output in binary format.

    ./convert2 xyzrgb
    
Used to convert a file in x, y, z, r, g, b format to a binary `.vxl` file.
This program contains some hard coded numbers which need to be tuned when converting a new file.

Orientation
-----------
The system uses a left-handed axis system. Upon loading the **Voxel-Engine**, 
the positive X direction is to the right,
the positive Y direction is to the top and
the camera is facing towards the positive Z direction.

File structures
---------------
The ASCII `.vxl.txt` file stores one point per line. 
Each line contain the (X,Y,Z) coordinate as decimal and the color as hexadecimal number. 
The numbers are space separated.

The binary `.vxl` file stores one point per 32 bytes. 
The structure of a point is given in `pointset.h`.

The binary `.oct` file stores an octree containing a model. 
It is a list of octree nodes, with the first one being the root.
Its structure is given in `octree.h`.

License
-------
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

