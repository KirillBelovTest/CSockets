# CSockets

- Linux - __Stable__
- MacOS - __Stable__
- Windows - __Stable__

## Examples

### Single page
```shell
wolframscript -f Tests/Simple.wls
```

### Dynamic app (involves websockets)
```shell
wolframscript -f Tests/Full.wls
```

## Building
In the `LibraryResurces` we placed all prebuild binaries.
__Skip this section if you want just to run this package__

If there are some issues with a shipped binaries, one can try to compile it.
```bash
wolframscript -f Scritps/BuildLibrary.wls
```

## Documentation

### tcp.c

socketOpen(hostname, port, opts...): listenSocketId;
socketClose(socketId): successState
socketConnect(hostname, port): clientSocketId
socketWriteString(socketId, string): successState
socketBinaryWrite(socketId, byteArray): successState
socketGetPort(socketId): portNumber
socketGetHostname(socketId): hostname

serverCreate
serverDestroy
serverAccept
serverRecv