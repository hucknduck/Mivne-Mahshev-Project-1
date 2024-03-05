	add $s0, $zero, $zero, $imm1, 0x100, 0x0	 # addi $s0 $0 0x100 | set the original addresses to the Matrices
	add $s1, $zero, $zero, $imm1, 0x110, 0x0	 # addi $s1 $0 0x110 | 
	add $s2, $zero, $zero, $imm1, 0x120, 0x0	 # addi $s2 $0 0x120 | 
LOOP:							 # loop logic as a "do while"
	beq $zero $zero $zero $imm2 0x0 VECMULT		 # j VECMULT | 
CHECK:							 # and set next addresses
	add $s2 $s2 $zero $imm1 0x001 0x0		 # addi $s2 $s2 1 | increment the target address
	and $t0 $s2 $imm1 $imm2 0xFFF 0x00C		 # and $t0 $s2 0xC | get i
	and $t1 $s2 $imm1 $imm2 0xFFF 0x003		 # and $t1 $s2 0x3 | get j
	add $s0 $zero $zero $imm1 0x100 0x0		 # addi $s0 $0 0x100 | reset address for Matrix A
	add $s1 $zero $zero $imm1 0x110 0x0		 # addi $s1 $0 0x110 | reset address for Matrix B
	or $s0 $s0 $t0 $zero 0x0 0x0			 # or $s0 $s0 $t0 | set address to A_i0
	or $s1 $s1 $t1 $zero 0x0 0x0			 # or $s1 $s1 $t1 | set address to B_0j
	beq $zero $s2 $imm1 $imm2 0x130 OUT		 # beq $s2 0x130 OUT | if we just set C_33, leave
	beq $zero $zero $zero $imm2 0x0 LOOP		 # j LOOP | else loop
VECMULT: 						 # multiply row vector in A with column vector in B
	add $t2 $zero $zero $zero 0x0 0x0		 # add $t2 $0 $0 | 
	lw $t0 $s0 $zero $zero 0x0 0x0			 # lw $t0 0($s0) | 
	lw $t1 $s1 $zero $zero 0x0 0x0			 # lw $t1 0($s1) | 
	mac $t2 $t0 $t1 $t2 0x0 0x0			 # mul $t0 $t0 $t1 | 
							 # add $t2 $t2 $t0 | # above command encapsulates mul and add
	lw $t0 $s0 $imm1 $zero 0x1 0x0			 # lw $t0 1($s0) | 
	lw $t1 $s1 $imm1 $zero 0x4 0x0			 # lw $t1 4($s1) | 
	mac $t2 $t0 $t1 $t2 0x0 0x0			 # mul $t0 $t0 $t1 | 
							 # add $t2 $t2 $t0 | # above command encapsulates mul and add
	lw $t0 $s0 $imm1 $zero 0x2 0x0			 # lw $t0 2($s0) | 
	lw $t1 $s1 $imm1 $zero 0x8 0x0			 # lw $t1 8($s1) | 
	mac $t2 $t0 $t1 $t2 0x0 0x0			 # mul $t0 $t0 $t1 | 
							 # add $t2 $t2 $t0 | # above command encapsulates mul and add
	lw $t0 $s0 $imm1 $zero 0x3 0x0			 # lw $t0 3($s0) | 
	lw $t1 $s1 $imm1 $zero 0xC 0x0			 # lw $t1 12($s1) | 
	mac $t2 $t0 $t1 $t2 0x0 0x0			 # mul $t0 $t0 $t1 | 
							 # add $t2 $t2 $t0 | # above command encapsulates mul and add
	sw $t2 $s2 $zero $zero 0x0 0x0			 # sw $t2 0($s2) | 
	beq $zero $zero $zero $imm2 0x0 CHECK		 # j CHECK | 
OUT:
	halt $zero $zero $zero $zero 0x0 0x0
	.word 256 1
	.word 257 2
	.word 258 3
	.word 259 4
	.word 260 5
	.word 261 6
	.word 262 7
	.word 263 8
	.word 264 9
	.word 265 10
	.word 266 11
	.word 267 12
	.word 268 13
	.word 269 14
	.word 270 15
	.word 271 16
	.word 272 1
	.word 273 2
	.word 274 3
	.word 275 4
	.word 276 1
	.word 277 2
	.word 278 3
	.word 279 4
	.word 280 1
	.word 281 2
	.word 282 3
	.word 283 4
	.word 284 1
	.word 285 2
	.word 286 3
	.word 287 4