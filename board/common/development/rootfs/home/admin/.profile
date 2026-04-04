# ~/.profile: executed by Bourne-compatible login shells.

export LANG=en_US.UTF-8
export TERM=linux

if [ -d /usr/lib/locale/en_US.utf8/ ]; then
  export LC_ALL=en_US.UTF-8
fi

if [ "$BASH" ]; then
  if [ -f ~/.bashrc ]; then
    . ~/.bashrc
  fi
fi
