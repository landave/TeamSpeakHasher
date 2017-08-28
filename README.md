# TeamSpeakHasher

TeamSpeakHasher is an OpenCL-based tool that can be used to increase the security level of TeamSpeak identities with unprecedented performance and efficiency.

## Build Instructions
### Linux
1. Make sure you have the OpenCL headers and libraries set up properly on your system.
2. Use the provided `Makefile` to build: `make all`
3. Enjoy.

### Windows
1. Make sure to have the environment variables `OPENCL_LIB_WIN32` and `OPENCL_LIB_WIN64` set to the directories of the 32-bit and 64-bit `OpenCL.lib` library, respectively.
2. Open the project file `TeamSpeakHasher.vcxproj` with Visual Studio.
3. Build.
4. Enjoy.
   
## Usage
The general usage format is as follows.

​```
./TeamSpeakHasher COMMAND [OPTIONS]​```

There are two commands:
* `add -publickey PUBLICKEY [-startcounter STARTCOUNTER] [-nickname NICKNAME]`

  Adds an identity to the database (stored in the file `tshasher.ini`).
  - `-publickey PUBLICKEY` is required. To find the public key of your identity (or to generate a good identity), you can use the [TSIdentityTool](https://github.com/landave/TSIdentityTool).
  - `-startcounter STARTCOUNTER` is optional. The passed `STARTCOUNTER` is the counter at which the computation begins. If you have already increased the security level of your identity, then you might want to use your current counter as `STARTCOUNTER`.
  - `-nickname NICKNAME` is optional. The passed `NICKNAME` will be used to represent the identity in the selection that is displayed in the `compute` command.
  
* `compute [-throttle throttlefactor] [-retune]`

  Starts the actual computation.
   - `-throttle throttlefactor` is optional. If it is provided, the global work size is divided by `throttlefactor`. This can be used to reduce the load on your GPU.
   - `-retune` is optional. If it is provided, the tuning algorithm is rerun and previously stored tuning parameters are overwritten.


## FAQ
* **How do I find the public key of my identity?**
  
    Use the [TSIdentityTool](https://github.com/landave/TSIdentityTool).
* **How can I export an identity with its best counter back to the TeamSpeak ini format?**

    Exporting to the TeamSpeak ini format is not supported by TeamSpeakHasher.
    This is because such an ini file would include the private key of your identity.
    Since TeamSpeakHasher's computation only depends on the public key, we do not even allow the tool to touch the private key.
    Consequently, you need to insert the best counter from TeamSpeakHasher to your TeamSpeak identity ini file.
    In order to do this, export your identity with TeamSpeak and open the generated ini file, which will contain an `identity` of the following format:
    
    `identity := COUNTER || 'V' || obfuscatedKeyPair`
    
    Now you can simply put the desired counter before the first `V` of your identity and reimport the ini file to TeamSpeak.
    
* **How can I remove or modify an identity from the database?**

    Removing or modifying database entries is currently not supported. In order to do this, please edit `tshasher.ini` manually.
* **What is the input string to the hash function?**

    The input string is `PUBLICKEY || COUNTER`, where `PUBLICKEY` is the public key passed to the TeamSpeakHasher's `add` command, and `COUNTER` is a decimal ASCII encoding of the current counter (64-bit unsigned integer).
    
    For more details to how the computation is done and how the public key relates to what is stored in the ini file, we refer to the [FAQ of TSIdentityTool](https://github.com/landave/TSIdentityTool#faq).
* **What is the slow phase?**
    
    The slow phase is reached when the input to the hash function has a length such that two blocks (instead of one block) need to be compressed every time the counter is increased. Hence, the computation in the slow phase is only half as fast.
    If you reach the slow phase, it is **strongly** recommended to switch to another identity. You can use the [TSIdentityTool](https://github.com/landave/TSIdentityTool) to generate identities that do (virtually) not suffer from this problem (so-called _good identities_).
* **Can I use my CPU to increase the security level?**

    Currently, only GPUs are supported. This is mainly due to the current code not being vectorized properly. However, OpenCL also supports CPUs. You can try to change `DEV_TYPE` in `TSHasherContext.h` to `CL_DEVICE_TYPE_CPU`.


## License

TeamSpeakHasher is licensed under the MIT license. See `LICENSE` for more information.
    
## Credits

An unoptimized SHA1 implementation (see `Kernel.h`) has been reused from the [hashcat](https://hashcat.net/hashcat/) project.