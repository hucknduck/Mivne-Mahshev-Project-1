	lw $s2, $imm2, $zero, $zero, 0x0, 0x100		 # $s2 = R
	mac $s2, $s2, $s2, $zero, 0x0, 0x0		 # $s2 = R^2
	add $s0, $zero, $zero, $zero, 0x0, 0x0		 # set $s0 to first pixel
	srl $s1, $imm1, $imm2, $zero, 0xFFF, 16		 # set $s1 to final pixel, 0xFFFF
	add $s1, $s1, $imm1, $zero, 1, 0		 # set $s1 to # of pixels
LOOP:							
	srl $t0, $s0, $imm1, $zero, 8, 0		 # $t0 = $s0[15:8] = rownum = Y
	and $t1, $s0, $imm1, $imm2, 0xFF, 0xFFF		 # $t1 = $s0[7:0] = columnnum = x
	sub $t0, $t0, $imm1, $zer0, 128, 0		 # normalize Y to center of screen
	sub $t1, $t1, $imm1, $zero, 128, 0		 # normalize X to center of screen
	mac $t2, $t1, $t1, $zero, 0, 0			 # $t2 = X^2
	mac $t2, $t0, $t0, $t2, 0, 0			 # $t2 = X^2 + Y^2	
	ble $zero, $t2, $s2, $imm2, 0x0, WHITE		 # if X^2 + Y^2 <= R^2, jump to White
	beq $zero, $zero, $zero, $imm2, 0x0, BLACK	 # else jump to BLACK
WHITE:
	out $zero, $imm1, $zero, $imm2, 21, 255		 # write 255 (white) to monitordata
	beq $zero, $zero, $zero, $imm2, 0x0, OUTPUT
BLACK:
	out $zero, $imm1, $zero, $imm2, 21, 0		 # write 0 (black) to monitordata
OUTPUT:
	out $zero, $imm1, $zero, $s0, 20, 0		 # write current pixel address to monitoraddr
	out $zero, $imm1, $zero, $imm2, 22, 1		 # write 1 to monitorcmd
	add $s0, $s0, $imm1, $zero, 1, 0		 # inc current pixel
	bne $zero, $s0, $s1, $imm2, 0, LOOP		 # LOOP if Current pixel is not # of pixels
EXIT:
	halt, $zero, $zero, $zero, $zero, 0, 0