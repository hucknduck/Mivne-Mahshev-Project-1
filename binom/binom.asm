	lw $s0, $imm1, $zero, $zero, 0x100, 0x0 		 # $s0 = n
	lw $s1, $imm1, $zero, $zero, 0x101, 0x0			 # $s1 = k
	add $ra, $imm1, $zero, $zero, OUT, 0x0
BINOM:
	### add $s0, $a0, $zero, $zero, 0x0, 0x0
	### add $s1, $a1, $zero, $zero, 0x0, 0x0 
	add $sp, $sp, $imm2, $zero, 0x0, -2
	sw $ra, $sp, $imm1, $zero, 0x1, 0x0
	sw $s2, $sp, $zero, $zero, 0x0, 0x0
	beq $zero, $s1, $zero, $imm2, 0x0, ONE			 # if k == 0
	beq $zero $s0, $s1, $imm2, 0x0, ONE			 # if n == k
	add $s0, $s0, $imm1, $zero, -1, 0x0			 # n--
	jal $ra, $zero, $zero, $imm2, 0x0, BINOM		 # call BINOM(n-1, k)
	add $s2, $v0, $zero, $zero, 0x0, 0x0			 # $s2 = BINOM(n-1,k)
	add $s1, $s1, $imm1, $zero, -1, 0x0			 # k--
	jal $ra, $zero, $zero, $imm2, 0x0, BINOM		 # call BINOM(n-1, k-1)
	add $s2, $s2, $v0, $zero, 0x0, 0x0			 # $s2 += BINOM(n-1, k-1)
	add $s0, $s0, $imm1, $zero, 0x1, 0x0			 # n++
	add $s1, $s1, $imm1, $zero, 0x1, 0x0			 # k++
	beq $zero, $zero, $zero, $imm2, 0x0, RET
ONE:
	add $s2, $zero, $zero, $imm2, 0x0, 0x1			 # set return value = 1
RET:
	add $v0, $s2, $zero, $zero, 0x0, 0x0			 # ret value = current sum
	lw $ra, $sp, $imm1, $zero, 0x1, 0x0
	lw $s2, $sp, $zero, $zero, 0x0, 0x0
	add $sp, $sp, $imm2, $zero, 0x0, 2
	beq $zero, $zero, $zero, $ra, 0, 0
OUT:
	sw $v0, $imm1, $zero, $zero, 0x102, 0x0
	halt $zero, $zero, $zero, $zero, 0x0, 0x0
	.word 256 10
	.word 257 2