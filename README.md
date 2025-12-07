# DISRecorder
Application to record and playback DIS (Distributed Interactive Simulation) traffic.

## Minimal Qt6/CMake example

Build and run:

1. Install OpenDIS locally (once):  
   `cmake --install /home/eidelen/dev/libs/open-dis-cpp/build --prefix /home/eidelen/dev/libs/open-dis-cpp/install`
2. Configure and build:  
   `cmake -S . -B build`  
   `cmake --build build`
3. Run: `./build/disrecorder`

If OpenDIS is installed elsewhere, point CMake to it with  
`-DOpenDIS_DIR=/path/to/OpenDIS/lib/cmake/OpenDIS`.

## Capture and replay DIS traffic

- The UI lets you set a capture file, multicast IP/port to listen on, and start/stop recording. Captured packets are stored with a simple binary header: `qint64 timestampMs`, `quint32 size`, followed by raw payload bytes (big endian via `QDataStream`).
- Replay uses its own file selector; set the replay destination IP/port and click “Start Replay”. Packets are emitted with the original inter-packet timing. Use “Loop replay” to continuously restart when the file ends.

## Add OpenDIS locally

Clone and build OpenDIS:
1. `git clone https://github.com/open-dis/open-dis-cpp.git /home/eidelen/dev/libs/open-dis-cpp`
2. `cmake -S /home/eidelen/dev/libs/open-dis-cpp -B /home/eidelen/dev/libs/open-dis-cpp/build`
3. `cmake --build /home/eidelen/dev/libs/open-dis-cpp/build`
4. Install to a local prefix:  
   `cmake --install /home/eidelen/dev/libs/open-dis-cpp/build --prefix /home/eidelen/dev/libs/open-dis-cpp/install`

Integrate in this project:
- Configure with `-DOpenDIS_DIR=/home/eidelen/dev/libs/open-dis-cpp/install/lib/cmake/OpenDIS`
  (or point to your prefix). CMake will pick up `OpenDIS::DIS6`/`OpenDIS::DIS7` targets.
