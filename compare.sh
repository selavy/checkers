#!/bin/sh

python ~/bin/github.com/raven-checkers/checkers.py > raventmp.txt
sort raventmp.txt > raven-sorted.txt

make -C ~/bin/github.com/checkers/
~/bin/github.com/checkers/checkers > mytmp.txt
sort mytmp.txt > my-sorted.txt

comm -3 raven-sorted.txt my-sorted.txt
