	out $zero, $imm1, $zero, $imm1, 1, 0 				#write to IO[1] 1 enable IRQ1
	out $zero, $imm1, $zero, $imm2, 6, ISR 				#write to IO[6] ISR address
	add $s1, $zero, $zero, $zero, 0, 0				#set (internal) diskstatus to 0 (busy)
	add $s0, $imm1, $zero, $zero, 7, 0				#set $s0 to first sector we will read
	out $zero, $imm1, $zero, $imm2, 16, 512				#arbitrarily write and read from mem address 512
LOOP:
	out $zero, $imm1, $zero, $s0, 15, 0				#read from sector $s0
	out $zero, $imm1, $zero, $imm2, 14, 1				#read
BLOCKONREAD:
	beq $zero, $s1, $zero, $imm1, BLOCKONREAD, 0			#loop until get interrupt
	add $s1, $zero, $zero, $zero, 0, 0				#set (internal) diskstatus to 0 (busy)
	add $t0, $s0, $imm1, $zero, 1, 0				#set $t0 to sector+1
	out $zero, $imm1, $zero, $t0, 15, 0				#write from sector $s0
	out $zero, $imm1, $zero, $imm2, 14, 2				#write
BLOCKONWRITE:
	beq $zero, $s1, $zero, $imm1, BLOCKONWRITE, 0			#loop until get interrupt
	add $s1, $zero, $zero, $zero, 0, 0				#set (internal) diskstatus to 0 (busy)
	add $s0, $s0, $imm1, $zero, -1, 0				#set next sector -1
	ble $zero, $zero, $s0, $imm1, LOOP, 
	halt $zero, $zero, $zero, $zero, 0, 0
ISR:
	out $zero, $imm1, $zero, $zero, 4, 0				#reset irq1status
	add $s1, $imm1, $zero, $zero, 1, 0				#set (internal) diskstatus to 1 (done)
	reti $zero, $zero, $zero, $zero, 0, 0	