# Init the pointers to the correct addresses
addi $s0 $0 0x100 | add $s0 $0 $0 $imm1 0x100 0x0
addi $s1 $0 0x110 | add $s1 $0 $0 $imm1 0x110 0x0
addi $s2 $0 0x120 | add $s2 $0 $0 $imm1 0x120 0x0

#loop logic as a "do while"

LOOP:
j VECMULT | beq $0 $0 $0 $imm2 0x0 VECMULT

CHECK: # and set next addresses
addi $s2 $s2 1 | add $s2 $s2 $0 $imm1 0x001 0x0

and $t0 $s2 0xC | and $t0 $s2 $imm1 $imm2 0xFFF 0x00C # 0, 4, 8, 12
and $t1 $s2 0x3 | and $t1 $s2 $imm1 $imm2 0xFFF 0x003 # mod4: 0, 1, 2, 3
addi $s0 $0 0x100 | add $s0 $0 $0 $imm1 0x100 0x0
addi $s1 $0 0x110 | add $s1 $0 $0 $imm1 0x110 0x0
or $s0 $s0 $t0 | or $s0 $s0 $t0 $0 0x0 0x0
or $s1 $s1 $t1 | or $s1 $s1 $t1 $0 0x0 0x0

beq $s2 0x12F OUT | beq $0 $s2 $imm1 $imm2 0x12F OUT
j LOOP | beq $0 $0 $0 $imm2 0x0 LOOP

# written in MIPS to translate to SIMP
# multiply row vector in M1 with column vector in M2
# before jumping to VECMULT have $s0 point to row in M1 and $s1 point to column in M2, and $s2 point to target value in result Matrix
VECMULT: 
add $t2 $0 $0 | add $t2 $0 $0 $0 0x0 0x0

lw $t0 0($s0) | lw $t0 $s0 $0 $0 0x0 0x0
lw $t1 0($s1) | lw $t1 $s1 $0 $0 0x0 0x0
mul $t0 $t0 $t1 | mac $t2 $t0 $t1 $t2 0x0 0x0
add $t2 $t2 $t0 | # above command encapsulates mul and add

lw $t0 1($s0) | lw $t0 $s0 $imm1 $0 0x1 0x0
lw $t1 4($s1) | lw $t1 $s1 $imm1 $0 0x4 0x0
mul $t0 $t0 $t1 | mac $t2 $t0 $t1 $t2 0x0 0x0
add $t2 $t2 $t0 | # above command encapsulates mul and add

lw $t0 2($s0) | lw $t0 $s0 $imm1 $0 0x2 0x0
lw $t1 8($s1) | lw $t1 $s1 $imm1 $0 0x8 0x0
mul $t0 $t0 $t1 | mac $t2 $t0 $t1 $t2 0x0 0x0
add $t2 $t2 $t0 | # above command encapsulates mul and add

lw $t0 3($s0) | lw $t0 $s0 $imm1 $0 0x3 0x0
lw $t1 12($s1) | lw $t1 $s1 $imm1 $0 0x12 0x0
mul $t0 $t0 $t1 | mac $t2 $t0 $t1 $t2 0x0 0x0
add $t2 $t2 $t0 | # above command encapsulates mul and add

sw $t2 0($s2) | sw $t2 $s2 $0 $0 0x0 0x0
j CHECK | beq $0 $0 $0 $imm2 0x0 CHECK


# done, leave
OUT:
halt $0 $0 $0 $0 0x0 0x0