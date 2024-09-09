#  CS2 DMA ESP Cheat
Welcome to the DmaCs2Esp repository, a simple yet powerful tool for DMA users.

## Features
-esp

# Getting Started
## Prerequisites
This program requires the following DLLs to be in the root directory when running(folder with the tool inside):

- FTD3XX.dll
- leechcore.dll
- vmm.dll
Download these from your DMA supplier.

You can find these additional files in the compiled version of ulfrisk.

## Libraries
This project depends on leechcore.lib and vmm.lib, which need to be placed in the DMALibrary -> libs folder. For security reasons, precompiled libraries are not included in this repository.

You can find the .libs in the include folders of both projects:

LeechCore,
MemProcFS



## Building the Project
Clone the repository to your local machine.
Download and place the required DLLs in the root directory.
Download the necessary libraries and place them in the libs/ folder.
Compile the project using your preferred IDE or build system.
Have fun.

## Update
To update the offsets, navigate to main.cpp and modify the values in the offsets namespace, starting at line 51.
(The offsets are old in this repo as i didnt use it for a long time)


## Credits
Special thanks to Metick for his assistance in the development of this cheat and for allowing the use of his library.
https://github.com/Metick/DMALibrary

## License
This DMALibrary is open-source and licensed under the MIT License. Feel free to use, modify, and distribute it in your projects.

## Disclaimer
This repository and the associated code are intended for educational and research purposes only. The use of this code for cheating in games or other unethical activities is strongly discouraged.
