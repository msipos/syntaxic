#!/bin/bash

INPUT_EXT=jruby
OUTPUT_EXT=rb
TARGET=results/ruby

mkdir -p $TARGET

for filename in input/*/* ; do
  filename=`echo $filename | grep -v "^results/" | grep "$INPUT_EXT$"`
  output=`echo $filename | sed -e "s/$INPUT_EXT$/$OUTPUT_EXT/"`
  if [ -z $filename ] ; then
    continue
    #echo "foo"
  fi
  
  if [ "$filename" != "$output" ] ; then
    mv $filename $output
  fi
  mv $output $TARGET
  echo $filename $output
done
