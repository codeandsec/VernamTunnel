## Vernam-Tunnel

Cross-platform multi-thread TCP tunnel with Vernam cipher encryption. You can use this software to enable remote access to your private services in your network.

## Compiling Example

Compile in *nix OS for *nix:
```
$ make clean
$ make

```

Compile in *nix OS for Windows:
```
$ apt-get install wine mingw32 mingw32-binutils mingw32-runtime
$ make clean
$ make -f Makefile.WinInLinux

```

Compile in Windows OS for Windows:
```
$ make clean
$ make -f Makefile.MinGW32

```


## Motivation

Having paranoia free private encrypted access to my NAS.

## Running

Listen on local port 443 and forward to NASServer:443, starting position of one time pad file: 6482691749

$ vernamtunnel --local-port=443 --remote-host=NASServer --remote-port=443 --key-file=/media/Encrypted --start-pos=6482691749

Listen on local host port 443, forward to MYINTERNETIP:18734, same starting position of one time pad file:

$ vernamtunnel.exe --local-port=443 --remote-host=MYINTERNETIP --remote-port=18734 --key-file=F:\Encrypted --start-pos=6482691749

Run without starting position parameter to start from beginning of file:

$ vernamtunnel.exe --local-port=443 --remote-host=MYINTERNETIP --remote-port=18734 --key-file=F:\Encrypted

## Read More

https://www.codeandsec.com/Poor-Mans-Unbreakable-Encrypted-TCP-Tunnel

## License

GPL licensed. See License.md file.
