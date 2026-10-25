#!/bin/sh
# usage: build.sh name   (builds name.c -> name.v32)
set -e
n=$1
v32cc --null 0 -g -o $n.asm $n.c
v32as -g --map $n.asm.debug -o $n.vbin $n.asm
cat > $n.xml <<XML
<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<rom-definition version="1.0">
    <rom type="cartridge" title="$n" version="1.0" />
    <binary path="$n.vbin" />
    <textures></textures>
    <sounds></sounds>
</rom-definition>
XML
packrom $n.xml -o $n.v32
