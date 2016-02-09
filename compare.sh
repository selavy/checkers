#!/bin/sh

python ~/bin/github.com/raven-checkers/checkers.py > raventmp.txt
sort raventmp.txt > raven-sorted.txt

~/bin/github.com/checkers/checkers > mytmp.txt
sort mytmp.txt > my-sorted.txt

diff raven-sorted.txt my-sorted.txt
