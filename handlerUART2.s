# Test stdin and stdout -- echo characters read from stdin to stdout
#   test ends when '%' (percent) is read/typed.

	.include "cMIPS.s"
	.text
	.align 2
	.set noreorder
	.globl _start
	.ent _start

        .set HW_stdout_addr,(x_IO_BASE_ADDR + 1 * x_IO_ADDR_RANGE)
        .set HW_stdin_addr, (x_IO_BASE_ADDR + 2 * x_IO_ADDR_RANGE)

	
_start:	nop
	la    $10, HW_stdin_addr
	la    $20, HW_stdout_addr
	li    $4,  '%'

snd:
	lw   $3, 0($10)		# get char from STDIN

	sw   $3, 0($20)		# send it to STDOUT

	beq  $3, $0, end	# got NUL?  go to end

	beq  $3, $4, end	# got '%'?  go to end

	j snd
	
end:
	wait 0
	.end _start
