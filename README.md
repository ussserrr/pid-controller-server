# pid-controller-server
Standalone proportional-integral-derivative regulator with the ability to be controlled via Wi-Fi (acts like UDP server).


## Overview
The app relies on the official [ESP-IDF](https://github.com/espressif/esp-idf) framework. Instruction set of the regulator itself can be find in [`commandmanager.h`](/components/commandmanager/include/commandmanager.h) file or in [pid-controller-gui](https://github.com/ussserrr/pid-controller-gui) repository. Supports both IPv4 and IPv6 networks.

`udp_server_task` serves main UDP server and constantly listening for incoming messages. They are then passed to  `commandmanager` module (`process_request()` function) and the prepared reply is sending back to the client.

`_stream_task` is an internal task only active when stream of process variable and controller output values is requested.

`pid` component performing the main PID algorithm.


## Usage
Refer to ESP-IDF [documentation](https://docs.espressif.com/projects/esp-idf/en/latest/index.html) for help on compile & run processes. Generally, to build, flash and run built-in UART monitor you should invoke:
```bash
$ make -j8 flash monitor
```
Alternatively, you can apply tasks of VSCode editor that have been used during development by me. They are placed in [.vscode/tasks.json](/.vscode/tasks.json) file.


## Client
The app can be easily paired with [pid-controller-gui](https://github.com/ussserrr/pid-controller-gui) PC utility out-of-the-box.


## TODOs
- [ ] Better port to more advanced PID engine such as [MiniPID](https://github.com/tekdemo/MiniPID) and use more classy architecture for managing PID configuration both in commandmanager and in dedicated task
