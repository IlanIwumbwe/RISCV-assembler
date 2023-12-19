.text
.globl main

main:
    JAL ra, init
    jal zero, main

init:
    li s2, 0x0   
    li s3, 0xfffffff  # load s3 with 0xff
    li s5, 0xffff   # result reg init at 0
    li s4, 0x8   

loopi:
    slli s2, s2, 1    # shift left by 1
    addi s2, s2, 1    # add 1
    and  s5, s3, s2

wait:
    addi s4, s4, -1
    BNE s4, zero, wait
    addi s4, zero, 0x8
    bne s3, s2, loopi
    ADDI s5, zero, 0
            