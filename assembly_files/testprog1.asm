.text
.globl main

main:
    JAL ra, init
    jal zero, main

init:
    ADDI s2, zero, 0x0   
    ADDI s3, zero, 0xff  # load s3 with 0x11111111
    addi s5, zero, 0x0   # result reg init at 0
    ADDI s4, zero, 0x8   

loopi:
    slli s2, s2, 1    # shift left by 1
    addi s2, s2, 1    # add 1
    and  s5, s3, s2
    beq  zero, zero, wait   

wait:
    addi s4, s4, -1
    BNE s4, zero, wait
    addi s4, zero, 0x8
    bne s3, s2, loopi
    ADDI s5, zero, 0

