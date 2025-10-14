
main:
    addi a0, zero, 0x100

    addi t0, zero, 0x1A5
    sb t0, 4(a0) # Should store 0xA5

    addi t1, zero, 0x1A5
    sh t1, 8(a0) # Should store 0x1A5

    addi t2, zero, -1
    sw t2, 12(a0) # Should store 0x00000FFF
    addi t3, zero, -1
    sw t3, 16(a0) # Ignnore for now

    lb a1, 4(a0) # Should load 0xA5
    lbu a2, 4(a0) # Should load 0xA5
    lh a3, 8(a0) # Should load 0x1A5
    lhu a4, 8(a0) 
    lw a5, 12(a0) 
    lw a6, 16(a0)
