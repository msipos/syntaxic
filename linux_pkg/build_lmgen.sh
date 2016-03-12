#!/bin/bash

g++ -std=c++11 -I src/ -o lmgen src/lmgenmain.cpp src/lmgen.cpp src/lm.cpp src/core/utf8_util.cpp