# ~/.profile: executed by Bourne-compatible login shells.

export LANG=en_US.UTF-8
export TERM=xterm-color

if [ "$BASH" ]; then
  if [ -f ~/.bashrc ]; then
    . ~/.bashrc
  fi
fi
