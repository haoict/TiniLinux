#!/bin/sh

CPU_FREQ=("/sys/devices/system/cpu/cpufreq/policy0")
GPU_FREQ=("/sys/devices/platform/fde60000.gpu/devfreq/fde60000.gpu")

get_threads() {
  for THREAD in $(seq 1 1 $(find /sys/devices/system/cpu -name online | wc -l)) all default
  do
    echo ${THREAD}
  done
}

set_online_threads() {
  AVAILABLE_THREADS=$(($(find /sys/devices/system/cpu -name online | wc -l) - 1))
  MODE=${2}
  if [ -z "${MODE}" ]
  then
    MODE=0
  fi
  case ${1} in
    default)
      return
    ;;
    all)
      THREADS=0
      MODE="1"
    ;;
    0)
      THREADS=1
    ;;
    *)
      THREADS=${1}
    ;;
  esac

  for thread in $(seq 0 1 ${THREADS})
  do
    echo 1  | tee /sys/devices/system/cpu/cpu${thread}/online >/dev/null 2>&1
  done

  for thread in $(seq ${THREADS} 1 ${AVAILABLE_THREADS})
  do
    echo ${MODE} | tee /sys/devices/system/cpu/cpu${thread}/online >/dev/null 2>&1
  done
}

set_cpu_gov() {
  for POLICY in $(ls /sys/devices/system/cpu/cpufreq 2>/dev/null | grep policy[0-9])
  do
    if [ -e "/sys/devices/system/cpu/cpufreq/${POLICY}/scaling_governor" ]
    then
      echo $1 >/sys/devices/system/cpu/cpufreq/${POLICY}/scaling_governor 2>/dev/null
    fi
  done
}

set_gpu_gov() {
  if [ -e "${GPU_FREQ}/governor" ]
  then
    for governor in $1 dmc_$1 simple_$1
    do
      echo ${governor} >${GPU_FREQ}/governor 2>/dev/null
      if [ "$?" = 0 ]
      then
        return
      fi
   done
  fi
}

onlinethreads() {
  set_online_threads ${1} ${2}
}

performance() {
  set_cpu_gov performance
  set_gpu_gov performance
}

ondemand() {
  set_cpu_gov ondemand
  set_gpu_gov ondemand
}

schedutil() {
  set_cpu_gov schedutil
  set_gpu_gov ondemand
}

powersave() {
  set_cpu_gov powersave
  set_gpu_gov powersave
}

case ${1} in
  performance)
    onlinethreads all
    performance
  ;;
  balanced_performance)
    onlinethreads all
    ondemand
  ;;
  balanced_powersave)
    onlinethreads 4
    powersave
  ;;
  powersave)
    onlinethreads 4
    powersave
  ;;
  *)
    # Default Settings
    onlinethreads all
    schedutil
  ;;
esac