#!/bin/bash
VIDEO_URL=$1
FORMAT=$2
if [ -z "$VIDEO_URL" ]; then
    echo "Usage: $0 <video_url> [format]"
    echo "Example: $0 https://www.youtube.com/watch?v=abcd1234 video"
    echo "Example: $0 https://www.youtube.com/watch?v=abcd1234 audio"
    exit 1
fi

if [ -z "$FORMAT" ] || [ "$FORMAT" == "video" ]; then
    FORMAT=18
fi
if [ "$FORMAT" == "audio" ]; then
    FORMAT=139
fi

if [ ! -f /usr/local/bin/yt-dlp ]; then
    wget https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp -O /usr/local/bin/yt-dlp
    chmod +x /usr/local/bin/yt-dlp
fi

# yt-dlp --list-formats $VIDEO_URL
# 18 - mp4 360p
# 139 - m4a 48k

yt-dlp -g -f $FORMAT $VIDEO_URL | xargs mpv --fs
