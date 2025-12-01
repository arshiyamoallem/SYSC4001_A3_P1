#!/bin/bash

if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi

g++ -g -O0 -I . -o bin/interrupts_EP interrupts_EP_101182048_101324189.cpp
g++ -g -O0 -I . -o bin/interrupts_RR interrupts_RR_101182048_101324189.cpp
g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_EP_RR_101182048_101324189.cpp