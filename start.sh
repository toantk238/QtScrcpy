#!/usr/bin/env zsh

cd /mnt/Data/Workspace/1.CloneAndRun/QtScrcpy

source ~/.zshrc;
source .envrc
./output/x64/debug/QtScrcpy 2>&1 | tee -a output.log
