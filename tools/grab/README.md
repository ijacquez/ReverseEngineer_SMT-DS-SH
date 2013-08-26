About
=====

### Description

`grab` is a small tool to quickly extract and output to a file, given a file offset and a size (in hexadecimal).

### Requirements
 - Python 2.7

Usage
=====

### Example #1

To grab 2,048 bytes from the file `foo` at offset 1024

    python grab 0x400 0x800 foo bar
