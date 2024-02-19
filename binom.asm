# init: set $a0 = n and $a1 = k V

# save $a0 and $a1 in $s0 and $s1 V
# dec stack by 2 V
# save return address and current sum ($s2) in stack V
# set $s2 = 0 V
# dec k, call binom, and save in $s reg
# dec n, call binom, and add to same $s reg
# write contents of $s reg to $v0
# inc n and k by 1
# restore $s reg and $ra from stack and inc stack by 2
# jump to $ra

	lw $s0, $imm1, $zero, $zero, 0x100, 0x0 		 # $s0 = n
	lw $s1, $imm1, $zero, $zero, 0x101, 0x0

BINOM:
	### add $s0, $a0, $zero, $zero, 0x0, 0x0
	### add $s1, $a1, $zero, $zero, 0x0, 0x0 
	add $sp, $sp, $imm2, $zero, 0x0, -2
	sw $ra, $sp, $imm1, $zero, 0x1, 0x0
	sw $s2, $sp, $zero, $zero, 0x0, 0x0
	add $s2, $zero, $zero, $zero, 0x0, 0x0
	add $

