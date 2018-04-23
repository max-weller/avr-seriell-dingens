#!/bin/sh -xe

cd ../esper-neu

app="$1"
version="$(cat dist/$app.version)"
echo "Compiling version $version ..."

make VERSION="$version" SITE="../esper-site" COM_PORT="$2" $app/clean $app $app/flash
