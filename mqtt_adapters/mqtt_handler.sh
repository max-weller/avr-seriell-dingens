#!/bin/bash
RC_SWITCH_DEVICE=/dev/arduinomicro
RC_SWITCH_DEVICE=/dev/ttyACM0
function define_var() {
mosquitto_pub -t 'ham/'$1'/$datatype' -m 'boolean' -r
mosquitto_pub -t 'ham/'$1'/$settable' -m 'true' -r
mosquitto_pub -t 'ham/'$1'/$name' -m "$2" -r
}
define_var "rc/1/on" "Decke"
define_var "rc/2/on" "Schreibtisch"
define_var "rc/3/on" "Regal"
define_var "rc/4/on" "Strahler"
define_var "rc/5/on" "Nachttisch"
define_var "lightsd/task/running" "Aktiv"
mosquitto_pub -t 'ham/lightsd/$name' -m 'Premiumfunzel'

mosquitto_pub -t 'ham/rc/$name' -m 'Remote Control' -r
mosquitto_pub -t 'ham/rc/$online' -m 'true' -r
mosquitto_sub --will-topic 'ham/rc/$online' --will-payload 'false' --will-retain -v -t ham/\# | while read line; do
echo $line
  case $line in
  "ham/lightsd/task/running true")
    pidof lightsd || (cd /home/mw/Projekte/elektro/ham/lightsd && ./build/lightsd) &
    ;;
  "ham/lightsd/task/running false")
    killall lightsd
    ;;
  "ham/rc/all/on/set true")
    echo 88f5d4b0 > $RC_SWITCH_DEVICE
    ;;
  "ham/rc/all/on/set false")
    echo 88f5d4e0 > $RC_SWITCH_DEVICE
    ;;
  ham/rc/[0-9]/on/set\ *)
  echo $line
    part="${line#ham/rc/}"
    switchNo="${part%/on/set *}"
    newState="${line#*/on/set }"
    if [ "$newState" = "true" ]; then
      mode=9
    else
      mode=8
    fi
    number="$(printf '%x' "$((16-$switchNo))")"
    echo $mode $number $newState
    echo 88f5d4$mode$number > $RC_SWITCH_DEVICE
    ;;
  ham/lights/set\ *)
    newState="${line#*/set }"
    mosquitto_pub -t ham/86a1f0/light/on/set -m "$newState"
    mosquitto_pub -t ham/06412c/light/on/set -m "$newState"
    mosquitto_pub -t ham/rc/all/on/set -m "$newState"
    mosquitto_pub -t ham/1e1792/red/on/set -m "$newState"
    mosquitto_pub -t ham/1e1792/yellow/on/set -m "$newState"
    mosquitto_pub -t ham/1e1792/green/on/set -m "$newState"
    ;;
  ham/door/alarm\ 1)
    mosquitto_pub -t ham/1e1792/red/set -m 1
    (sleep 30; mosquitto_pub -t ham/1e1792/red/set -m 0) &
    perl -e 'print "\xff\x11\x22"x78;' >/dev/udp/10.83.42.196/1337
    (echo "Türe wurde geöffnet" | /usr/local/bin/syslog_pushover.py) &
    ;;
  esac
done




