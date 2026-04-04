#!/bin/bash

##################################################################
# Created by Christian Haitian for use to display a python       #
# based keyboard to the user.                                    #
##################################################################

# Offline install urwid==2.1.2
# With internet connection, we can run  pip install urwid==2.1.2
# But for offline env, we need to install it from .whl file
# Steps to create urwid-2.1.2-py3-none-any.whl (from another handheld machine with internet access):
# 1. Run: pip download urwid==2.1.2 --dest /root/urwid_offline
# 2. Run: cd /root/urwid_offline && tar xzf urwid-2.1.2.tar.gz && cd urwid-2.1.2
# 3. Run: python3 setup.py bdist_wheel
# 4. The .whl file will be created in the dist/ folder: dist/urwid-2.1.2-py3-none-any.whl
# 5. Copy the .whl file to the target system and install it using pip:
#    pip install urwid-2.1.2-py3-none-any.whl


# Check /usr/share/lib/python/urwid-2.1.2-py3-none-any.whl exists, then install it
if [ -f /usr/share/lib/python/urwid-2.1.2-py3-none-any.whl  ]; then
  echo "Installing the python3 urwid module needed for this.  Please wait..." 2>&1 >/dev/tty
  pip install /usr/share/lib/python/urwid-2.1.2-py3-none-any.whl 2>&1 >/dev/tty
  if [ $? -eq 0 ]; then
    rm -f /usr/share/lib/python/urwid-2.1.2-py3-none-any.whl

    # Patch the urwid module in Python 3.14
    python3 -c "
      path = '/usr/lib/python3.14/site-packages/urwid/raw_display.py'
      with open(path, 'r') as f:
          lines = f.readlines()

      # Replace lines 669, 670, 671 (indices 668, 669, 670)
      lines[668] = '                import subprocess; r = subprocess.run([\"stty\", \"size\"], capture_output=True, text=True)\n'
      lines[669] = ''
      lines[670] = '                y, x = (int(v) for v in r.stdout.split()) if r.stdout.strip() else (24, 80)\n'

      with open(path, 'w') as f:
          f.writelines(lines)
      print('Done')
      "
  else
    echo "urwid module installation failed" 2>&1 >/dev/tty
  fi
fi

export XDG_RUNTIME_DIR=/run/user/$UID/
ps aux | grep gptokeyb2 | grep -v grep | awk '{print $1}' | xargs kill -9 >/dev/null 2>&1 || true
SDL_GAMECONTROLLERCONFIG_FILE="/usr/share/gamecontrollerdb.txt" /usr/local/bin/gptokeyb2 -c "/usr/share/gptokeyb2.ini" >/dev/null 2>&1 &

RESULTS="$(python3 /usr/local/bin/osk.py "$1" 2>&1 >/dev/tty)"
EXIT_CODE=$?
RESULTS="$(echo $RESULTS | tail -n 1)"
ps aux | grep gptokeyb2 | grep -v grep | awk '{print $1}' | xargs kill -9 >/dev/null 2>&1 || true

echo "$RESULTS"
exit $EXIT_CODE