#!/bin/bash

# Copyright (c) 2021
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
# Authored by: Kris Henriksen <krishenriksen.work@gmail.com>
# Thanks to Quack for modifications to account for SSIDs with spaces
#
# Wi-Fi-dialog
#

export XDG_RUNTIME_DIR=/run/user/$UID/

height="13"
width="55"

old_ifs="$IFS"
scan_cache=""

###################################
# Wifi functions
###################################
getCurrentConnectedSSID() {
  echo $(sudo nmcli -t -f name,device connection show --active | grep wlan0 | cut -d\: -f1)
}

deleteConnection() {
  dialog --clear --title "Removing $1" --yesno "\nWould you like to continue to remove this connection?" 8 $width 2>&1 >/dev/tty
  if [[ $? != 0 ]]; then
    return 0
  fi
  case $? in
  0) sudo nmcli con down "$1" >/dev/null 2>&1; sudo rm -f "/etc/NetworkManager/system-connections/$1.nmconnection" ;;
  esac

  DeleteMenu
}

connectExisting() {
  dialog --infobox "\nConnecting to: $1 ..." 5 $width 2>&1 >/dev/tty

  sudo nmcli con down "$1" >/dev/null 2>&1;
  sleep 1

  output=$(sudo nmcli con up "$1")
  success=$(echo "$output" | grep successfully)

  if [ -z "$success" ]; then
    dialog --infobox "\nFailed to connect to $1\n$output" $height $width 2>&1 >/dev/tty
    sleep 3
    ActivateExistingMenu
  else
    NetworkInfo "Successfully connected to: "
    MainMenu
  fi
}

makeConnection() {
  ps aux | grep gptokeyb2 | grep -v grep | awk '{print $1}' | xargs kill -9 >/dev/null 2>&1 || true
  PASS=$(/usr/local/bin/osk.sh "Enter Wi-Fi password for ${1:0:15}")
  EXIT_CODE=$?
  SDL_GAMECONTROLLERCONFIG_FILE="/usr/share/gamecontrollerdb.txt" /usr/local/bin/gptokeyb2 -c "/usr/share/gptokeyb2.ini" >/dev/null 2>&1 &
  if [[ $EXIT_CODE != 0 ]]; then
    ScanAndConnectMenu "use_scan_cache"
  fi

  PASS="$(echo $PASS | tail -n 1)"
  dialog --infobox "\nConnecting to: $1 ..." 5 $width 2>&1 >/dev/tty

  # try to connect
  sudo nmcli con delete "$1" >/dev/null 2>&1 || true
  output=$(sudo nmcli device wifi connect "$1" password "$PASS")
  success=$(echo "$output" | grep successfully)

  if [ -z "$success" ]; then
    sudo rm -f /etc/NetworkManager/system-connections/"$1".nmconnection
    dialog --msgbox "\nActivation failed: $output" 9 $width 2>&1 >/dev/tty
    ScanAndConnectMenu "use_scan_cache"
  else
    NetworkInfo "Successfully connected to: "
    MainMenu
  fi
}

###################################
# Menu
###################################

MainMenu() {
  if [[ ! -z $(rfkill -n -o TYPE,SOFT | grep wlan) ]]; then
    if [[ ! -z $(rfkill -n -o TYPE,SOFT | grep wlan | grep -w unblocked) ]]; then
      local Wifi_Stat="On"
      local Wifi_MStat="Off"
    else
      local Wifi_Stat="Off"
      local Wifi_MStat="On"
      mainMenuTitle="Wifi Disabled"
    fi

    mainoptions=(1 "Turn Wifi $Wifi_MStat (Currently: $Wifi_Stat)" 2 "Connect to new Wifi connection" 3 "Activate existing Wifi Connection" 4 "Delete exiting connections" 5 "Current Network Info" 6 "Exit")
  else
    local Wifi_Stat="Off"
    mainMenuTitle="No Wifi device found"
    mainoptions=(1 "Current Network Info" 2 "Exit")
  fi

  if [ $Wifi_Stat == "On" ]; then
    currentConnectedSSID=$(getCurrentConnectedSSID)
    if [[ -z $currentConnectedSSID ]]; then
      mainMenuTitle="Not connected"
    else
      mainMenuTitle="Currently connected to \"$currentConnectedSSID\""
    fi
  fi

  echo "Main Menu Title: $mainMenuTitle"


  IFS="$old_ifs"
  while true; do
    mainselection=(dialog --backtitle "Wifi Manager: $mainMenuTitle"
      --title "Main Menu"
      --no-collapse
      --clear
      --cancel-label "Exit"
      --menu "Please make your selection" $height $width 15)

    mainchoices=$("${mainselection[@]}" "${mainoptions[@]}" 2>&1 >/dev/tty)
    if [[ $? != 0 ]]; then
      exit 1
    fi

    if [[ $mainMenuTitle == "No Wifi device found" ]]; then
      for mchoice in $mainchoices; do
        case $mchoice in
        1) NetworkInfo ;;
        2) ExitMenu ;;
        esac
      done
    else
      for mchoice in $mainchoices; do
        case $mchoice in
        1) ToggleWifi $Wifi_MStat ;;
        2) ScanAndConnectMenu ;;
        3) ActivateExistingMenu ;;
        4) DeleteMenu ;;
        5) NetworkInfo ;;
        6) ExitMenu ;;
        esac
      done
    fi
  done
}

ToggleWifi() {
  dialog --infobox "\nTurning Wifi $1, please wait..." 5 $width 2>&1 >/dev/tty
  if [[ ${1} == "Off" ]]; then
    sudo systemctl stop NetworkManager
    sudo systemctl disable NetworkManager
    sudo rfkill block wlan
  else
    sudo rfkill unblock wlan
    sudo systemctl enable NetworkManager
    sudo systemctl start NetworkManager
    sleep 5
  fi
  MainMenu
}

ActivateExistingMenu() {
  currentConnectedSSID=$(getCurrentConnectedSSID)
  declare aoptions=()
  while IFS= read -r -d $'\n' ssid; do
    if [[ $currentConnectedSSID == $ssid ]]; then
      aoptions+=("$ssid" "*")
    else
      aoptions+=("$ssid" " ")
    fi
  done < <(ls -1 /etc/NetworkManager/system-connections/ | sed 's/.\{13\}$//' | sed -e 's/$//')

  while true; do
    aselection=(dialog --title "Which existing connection would you like to connect to?"
      --no-collapse
      --clear
      --cancel-label "Back"
      --menu "" $height $width 15)

    achoice=$("${aselection[@]}" "${aoptions[@]}" 2>&1 >/dev/tty) || MainMenu
    if [[ $? != 0 ]]; then
      exit 1
    fi

    # There is only one choice possible
    connectExisting "$achoice"
  done
}

ScanAndConnectMenu() {
  if [[ -z $1 ]]; then
    dialog --infobox "\nScanning available Wi-Fi access points..." 5 $width 2>&1 >/dev/tty
    clist=$(sudo nmcli -f ALL --mode tabular --terse --fields IN-USE,SSID,CHAN,SIGNAL,SECURITY dev wifi)
    if [ -z "$clist" ]; then
      clist=$(sudo nmcli -f ALL --mode tabular --terse --fields IN-USE,SSID,CHAN,SIGNAL,SECURITY dev wifi)
    fi
    scan_cache="$clist" # cache the scan result
  else
    clist="$scan_cache" # restore from cache
  fi

  # Set colon as the delimiter
  IFS=':'
  unset coptions
  while IFS= read -r line; do
    # Read the split words into an array based on colon delimiter
    read -a strarr <<<"$line"

    INUSE=$(printf '%-5s' "${strarr[0]}")
    SSID="${strarr[1]}"
    CHAN=$(printf '%-5s' "${strarr[2]}")
    SIGNAL=$(printf '%-5s' "${strarr[3]}%")
    SECURITY="${strarr[4]:-OPEN}"

    if [[ -n $SSID ]]; then
      coptions+=("$SSID" "$INUSE $CHAN $SIGNAL $SECURITY")
    fi
  done <<<"$clist"

  while true; do
    cselection=(dialog --title "SSID  IN-USE  CHANNEL  SIGNAL  SECURITY"
      --no-collapse
      --clear
      --cancel-label "Back"
      --menu "" 16 $width 15)

    cchoices=$("${cselection[@]}" "${coptions[@]}" 2>&1 >/dev/tty) || MainMenu
    if [[ $? != 0 ]]; then
      exit 1
    fi

    for cchoice in $cchoices; do
      case $cchoice in
      *) makeConnection $cchoice ;;
      esac
    done
  done
}

DeleteMenu() {
  currentConnectedSSID=$(getCurrentConnectedSSID)
  declare deloptions=()
  while IFS= read -r -d $'\n' ssid; do
    if [[ $currentConnectedSSID == $ssid ]]; then
      deloptions+=("$ssid" "*")
    else
      deloptions+=("$ssid" " ")
    fi
  done < <(ls -1 /etc/NetworkManager/system-connections/ | sed 's/.\{13\}$//' | sed -e 's/$//')

  while true; do
    delselection=(dialog
      --title "Which connection would you like to delete?"
      --no-collapse
      --clear
      --cancel-label "Back"
      --menu "" $height $width 15)

    # There is only a single choice possible
    delchoice=$("${delselection[@]}" "${deloptions[@]}" 2>&1 >/dev/tty) || MainMenu
    if [[ $? != 0 ]]; then
      exit 1
    fi
    deleteConnection "$delchoice"
  done
}

NetworkInfo() {
  gateway=$(ip r | grep default | awk '{print $3}')
  currentConnectedSSID=$(getCurrentConnectedSSID)
  if [[ -z $currentConnectedSSID ]]; then
    connectionName="Ethernet Connection: eth0"
    currentip=$(ip -f inet addr show eth0 | sed -En -e 's/.*inet ([0-9.]+).*/\1/p')
  else
    connectionName="SSID: $currentConnectedSSID"
    currentip=$(ip -f inet addr show wlan0 | sed -En -e 's/.*inet ([0-9.]+).*/\1/p')
  fi
  
  currentdns=$( (sudo nmcli dev list || sudo nmcli dev show) 2>/dev/null | grep DNS | awk '{print $2}')
  message=$1
  details=$(ip a | sed 's/$/\\n/')
  
  dialog --clear --title "Network Information" --msgbox "$message\n$connectionName\nIP: $currentip\nGateway: $gateway\nDNS: $currentdns\n\n\nDetails:\n$details" $height $width 2>&1 >/dev/tty
  if [[ $? != 0 ]]; then
    exit 1
  fi
}

# CountryMenu() {
#   cur_country=$(sudo iw reg get | grep country | cut -c 9-10)
#   if [[ "$cur_country" == "00" ]]; then
#     cur_country="WORLD"
#   fi
#   declare coptions=()
#   coptions=("WORLD" . "US" . "DZ" . "AR" . "AU" . "AT" . "BH" . "BM" . "BO" . "BR" . "BG" . "CA" . "CL" . "CN" . "CO" . "CR" . "CY" . "CZ" . "DK" . "DO" . "EC" . "EG" . "SV" . "EE" . "FI" . "FR" . "DE" . "GR" . "GT" . "HN" . "HK" . "IS" . "IN" . "ID" . "IE" . "PK" . "IL" . "IT" . "JM" . "JP3" . "JO" . "KE" . "KW" . "KW" . "LB" . "LI" . "LI" . "LT" . "LT" . "LU" . "MU" . "MX" . "MX" . "MA" . "MA" . "NL" . "NZ" . "NZ" . "NO" . "OM" . "PA" . "PA" . "PE" . "PH" . "PL" . "PL" . "PT" . "PR" . "PR" . "QA" . "KR" . "RO" . "RU" . "RU" . "SA" . "CS" . "SG" . "SK" . "SK" . "SI" . "SI" . "ZA" . "ES" . "LK" . "CH" . "TW" . "TH" . "TH" . "TT" . "TN" . "TR" . "UA" . "AE" . "GB" . "UY" . "UY" . "VE" . "VN" .)
#   while true; do
#     cselection=(dialog
#       --backtitle "Country currently set to $cur_country"
#       --title "Which country would you like to set your wifi to?"
#       --no-collapse
#       --clear
#       --cancel-label "Back"
#       --menu "" $height $width 15)
#     cchoice=$("${cselection[@]}" "${coptions[@]}" 2>&1 >/dev/tty) || MainMenu
#     if [[ $? != 0 ]]; then
#       exit 1
#     fi
#     # There is only one choice possible
#     if [[ "$cchoice" == "WORLD" ]]; then
#       sudo iw reg set 00
#     else
#       sudo iw reg set "$cchoice"
#     fi
#     CountryMenu
#   done
# }

ExitMenu() {
  dialog --clear
  exit 0
}

###################################
# Start MainMenu
###################################
dialog --clear
trap ExitMenu EXIT
MainMenu
