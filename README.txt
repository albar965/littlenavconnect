Little Navconnect
=================

Little Navconnect is a free open source application that acts as an agent connecting Little Navmap
with a FSX, Prepar3D or X-Plane flight simulator.

This allows to use Little Navmap on Linux or Mac and saves the the pain of setting up remote SimConnect links.

Network scenario for FSX or Prepar3D using SimConnect:

|Windows Machine                                   |     Windows, Linux or Mac Machines  |
------------------------------------------------------------------------------------------
FSX/P3D <-> Simconnect <-> Little Navonnect <- (Network) -> Little Navmap on Computer 1
                                            <- (Network) -> Little Navmap on Computer 2

Network scenario for X-Plane using the Little Xpconnect plugin:

|Windows, Linux or Mac Machine                           |     Windows, Linux or Mac Machines  |
-----------------------------------------------------------------------------------------------
X-Plane <-> Little XpConnect <-> Little Navonnect <- (Network) -> Little Navmap on Computer 1
                                                  <- (Network) -> Little Navmap on Computer 2

------------------------------------------------------------------------------

See the Little Navconnect help for more information. All online here: https://www.gitbook.com/@albar965

Little Navconnect supports FSX, FSX Steam Edition, Prepar3d Versions 2, 3, 4 and X-Plane

------------------------------------------------------------------------------
-- INSTALLATION --------------------------------------------------------------
------------------------------------------------------------------------------

Installation involves the simple copying of files therefore an installer or setup program is not required.

Do not extract the archive into the folder "c:\Program Files\" or "c:\Program Files (x86)\"
since you will need administrative privileges in some Windows versions.

Since Windows keeps control of these folders other problems might occur like replaced or deleted files.

Extract the Zip archive into a folder like "c:\Own Programs\Little Navconnect".
Then start the program by double-clicking "littlenavconnect.exe".

It is recommended to delete the any old installation directories of Little Navconnect.
You can also install a newer version into another directory but do not merge the installations.

Anyway, no settings are a stored in the installation directory. Therefore it is safe to remove it.

In some cases you have to install the MS Visual C++ 2013 Redistributable package
(https://www.microsoft.com/en-US/download/details.aspx?id=40784). Install both 32 and 64 bit versions.
Usually this is already installed since many other programs require it.

See in the online manual if installing for other Simulators than FSX SP2.

See the online manual for more details.

The installation on Linux and macOS computers is simlar except different paths.

------------------------------------------------------------------------------
-- LICENSE -------------------------------------------------------------------
------------------------------------------------------------------------------

This software is licensed under GPL3 or any later version.

The source code for this application is available at Github:
https://github.com/albar965/atools
https://github.com/albar965/littlenavconnect

Copyright 2015-2019 Alexander Barthel (alex@littlenavmap.org).

-------------------------------------------------------------------------------
French translation copyright 2017 Patrick JUNG alias Patbest (patrickjung@laposte.net).

