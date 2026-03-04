#!/bin/sh

CPU_FREQ=("/sys/devices/system/cpu/cpufreq/policy0")

set_cpu_gov() {
  for POLICY in $(ls /sys/devices/system/cpu/cpufreq 2>/dev/null | grep policy[0-9])
  do
    if [ -e "/sys/devices/system/cpu/cpufreq/${POLICY}/scaling_governor" ]
    then
      echo $1 >/sys/devices/system/cpu/cpufreq/${POLICY}/scaling_governor 2>/dev/null
    fi
  done
}


performance() {
  set_cpu_gov performance
}

ondemand() {
  set_cpu_gov ondemand
}

schedutil() {
  set_cpu_gov schedutil
}

powersave() {
  set_cpu_gov powersave
}

case ${1} in
  performance)
    performance
  ;;
  balanced_performance)
    ondemand
  ;;
  balanced_powersave)
    powersave
  ;;
  powersave)
    powersave
  ;;
  *)
    schedutil
  ;;
esac