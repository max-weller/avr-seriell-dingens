#!/bin/sh -xe

cd ../esper-neu

app="$1"
old_version="$(cat dist/$app.version || echo 0.0.0)"
version="${old_version%.*}.$((${old_version##*.}+1))"
echo "Compiling version $version ..."

make VERSION="$version" SITE="../esper-site" $app/clean $app

scp  dist/$app.* 10.83.42.11:/home/mw/Web/firmware/

mosquitto_pub -t ham/update -m "$app = $version"
mosquitto_pub -t ham/\$broadcast/update -m "$app = $version"
