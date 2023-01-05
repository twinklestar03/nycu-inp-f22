 .section ".rodata"
 .globl dict
 .type dict, STT_OBJECT
 .globl dict_size
 .type dict_size, STT_OBJECT
dict:
 .incbin "dictionary"
 .byte 0
 .size dict, .-dict
dict_size:
 .int(.-dict)
