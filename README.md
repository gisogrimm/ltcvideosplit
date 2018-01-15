# ltcvideosplit
Time-align video files based on the linear time code embedded in the audio track

This tool depends on libltc (https://github.com/x42/libltc) and ffmpeg
(libavutil libavformat libavcodec). On Ubuntu 14.04 these may be
installed with

sudo apt-get install libavutil-dev libavcodec-dev libavformat-dev libltc-dev



The original plan was to split video files into chunks; however, this
turned out to be more complex, thus this version just reports the
alignment frame numbers.

## Workflow

  1. Set up a LTC source with the frame rate matching your camera, e.g., using Ardour
  2. Connect this LTC audio signal with the audio input of your camera. You might need to control the recording gain and LTC level manually to avoid clipping or insufficient gain. Connection to several cameras is also possible. Record LTC signal on the first audio channel (or in mono).
  3. Configure all cameras to the same frame rate.
  4. Start the transport of your LTC source (e.g., start Ardour transport) and start your movie recording on your cameras.
  5. Download the video files to your computer, and call ltcvideosplit for each video file. A list of video input frame numbers and corresponding LTC frame numbers ist shown for all frames where the synchronization changed.
  6. Split your video file at the video input frame numbers, and time-align video snipplets to the listed LTC frame number in the video editor of your choice (e.g., blender).

Hint: When synchronizing the blender video editor to jack, it starts
counting frames at 1, whereas the Ardour-generated time code starts at
0.

For videos with 50 fps, use the option '-s 2' to skip every second
video frame in the analysis.
