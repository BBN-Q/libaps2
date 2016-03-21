#!/bin/sh
version=$(git describe --dirty)
basedir=$(dirname $0)
sed "s/<TOKEN>/$version/g" $basedir/lib/version_template.hpp > $basedir/lib/version.hpp
