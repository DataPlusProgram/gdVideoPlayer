# gdFFMPEG

This is a Godot FFMPEG implementation via GDNative.    
  
The implementation writes directly to a TexutreRect so is not super optimal.  
  
**Currently this project has only been tested for Windows** and only the Windows binaries are provided here.  
The binaries for other platforms will have to be manually compiled by the user.
  
This should work for any format the FFMPEG supports but I've not extensively tested it there are probably many incompatibilities not yet found.  
  
The most common error is bad audio playback under certain conditions.  
  
![](https://i.imgur.com/TRur6KL.png)
  
## Usage
Refer to the provided example project for usage(provided for Windows only)  

![](https://i.imgur.com/k4yM430.png)
