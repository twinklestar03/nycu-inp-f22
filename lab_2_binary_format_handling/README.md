# INP111 Lab02 Week #3 (2022-09-29)

Date: 2022-09-29

[TOC]

# #1 PAKO File Unpacker

This lab aims to practice binary file reading and handle binary data in data structure.

## Structure of PAKO file
* A PAKO file contains four sections: header section, file entries, string section, and content section.
* header section 
    * 4 bytes - magic number: `P`, `A`, `K`, `O`
    * 4 bytes - file offset of the string section
    * 4 bytes - file offset of the content section
    * 4 bytes - number of files packed in this file
* file entries
    * array of `FILE_E` (see the descriptions below) contains the information about each file.
* string section
    * The string section stores filename strings used in this file, each string is ended with a **null byte**.
* content section
    * The content section stores the content of all packed files.
### `FILE_E` data structure
* 4 bytes - offset of the filename string in the string section
* 4 bytes - file size (**big-endian**)
* 4 bytes - offset of the file content in the content section
* 8 bytes - checksum (**big endian**)
   *  Split the content of a file into segments of 8 bytes. The total number of segments should be $\lceil(\mbox{size-of-PAKO-file})/8\rceil$.
   *  Consider each segment as an ``uint64_t`` integer. The checksum is the **XOR** value of all segments.

## Sample PAKO File
* The sample file can be downloaded from [example.pak](https://inp111.zoolab.org/lab02.1/example.pak).
* There are three files packed in the sample PAKO file. The filenames are:
    * ``f1.txt``
    * ``f2``
    * ``f3.txt``
* The sample PAKO file's hexdump is shown below.<br/>
![](/uploads/upload_bef6eabc81940efa89e5dc1e2f4b1502.png)<br/><br/>
* `[0x0, 0x4)`: `50414b4f` is the magic number
* `[0x4, 0x8)`: `0x0000004c` is offset of the string section in this file. (The section starts with `66312e74`...... in this example.)
* `[0x8, 0xc)`: `0x00000060` is offset of the content section in this file. (The section starts with `00000000`...... in this example.)
* `[0xc, 0x10)`: `0x00000003` is the number of files packed in the sample PAKO file.
* Offsets `[0x10, 0x24)`, `[0x24, 0x38)`, `[0x38, 0x4c)`: The three `FILE_E` structures for the three files packed in this PAKO file, respectively.
* Take the second file as an example:
    * `[0x24, 0x38)`: the `FILE_E` structure of the second file (named `f2`).
        * `[0x24, 0x28)`: `0x00000007` indicates the filename string's offset in the string section. As a result, the filename string is located at offset `0x4c+0x7=0x53`. The string at offset `0x53` is `f2` (ended with a null byte).
        * `[0x28, 0x2c)`: `0x0000001c` (big endian) is the size of the file.
        * `[0x2c, 0x30)`: `0x00000020` is the offset of ``f2``'s content in the content section. The content of the file is located at offset `0x60+0x20=0x80`. (start with `66320a0a6632636f`......)
        * `[0x30, 0x38)`: `0x232c7d730f070a44` is the checksum of the file. `checksum = 0x6f6332660a0a3266 ^ 0x2e2e2e746e65746e ^ 0x62616161610a2e2e ^ 0x0a626262`. If the length of the last segment is less than 8 bytes, pad zeros at the end of the segment to ensure its length is correct.
    * `[0x53, 0x55]`: filename string of the second file.
    * `[0x80, 0x9c)`: file content of the second file.

## Steps

1. Implement an unpacker program in C/C++ to unpack the PAKO file. You should invoke your program using the following command: `./unpacker <src.pak> <dst>`, where `src.pak` is the input PAKO file and `dst` is the destination directory. You should unpack the files packed in the PAKO file to `dst` directory. When your program unpacks the files, it must calculate the checksum of each file. **Do not unpack the files having incorrect checksums.**

1. Please test your program with [example.pak](https://inp111.zoolab.org/lab02.1/example.pak)
    * Note that the checksum of `f3.txt` in this example is incorrect. Therefore, your program should not unpack `f3.txt`.

:::warning
You may need the following functions/headers to complete this lab.
* functions
    * [open(2)](https://man7.org/linux/man-pages/man2/open.2.html)
    * [read(2)](https://man7.org/linux/man-pages/man2/read.2.html)
    * [write(2)](https://man7.org/linux/man-pages/man2/write.2.html)
    * [lseek(2)](https://man7.org/linux/man-pages/man2/lseek.2.html)
* headers
    * <stdint.h>

Reference: https://man7.org/linux/man-pages/
:::

## Hints

* It is easier to handle binary data by reading byte streams into a user-defined data structure. For example, the data structure for the header part can be defined as
``` c
typedef struct {
        uint32_t magic;
        int32_t  off_str;
        int32_t  off_dat;
        uint32_t n_files;
} __attribute((packed)) pako_header_t;
```

## Demo
<!---
1. Download the test case from [here](https://inp111.zoolab.org/lab02.1/testcase.pak).
--->
1. Use the following command to test your program.
    ```
    wget https://inp111.zoolab.org/lab02.1/testcase0.pak
    wget https://inp111.zoolab.org/lab02.1/testcase.pak
    mkdir /tmp/inplab2test
    ./unpacker testcase.pak /tmp/inplab2test
    cd /tmp/inplab2test
    chmod +x checker
    ./checker
    ```
### Checkpoints
1. [20%] Your program shows the number of files packed in [`testcase0.pak`](https://inp111.zoolab.org/lab02.1/testcase0.pak).
4. [30%] Your program shows the information (filename, file size in bytes) of each file packed in [`testcase0.pak`](https://inp111.zoolab.org/lab02.1/testcase0.pak).
5. [20%] The `checker` program extracted from [`testcase.pak`](https://inp111.zoolab.org/lab02.1/testcase.pak) can run without a crash.
6. [30%] The `checker` program extracted from [`testcase.pak`](https://inp111.zoolab.org/lab02.1/testcase.pak) can run and show the `Bingo` message successfully.

:::warning
It would be better to remove all files in the output directory before you run your unpacking program. Once unpacked, you must run the `checker` program inside the output directory.
:::
