	.arch msp430g2553
	.align 1,0
	.text

	.data 	; s is a static variable (in ram)
sw:	       .word 1
start:	       .word 1


	         .text 		; jt is constants (in flash)
jt:	      .word default	;  jt[0]
	         .word play	;  jt[1]
	         .word stop	;  jt[2]

	         .global stateMove
stateMove:

;;;  range check on selector (s)
	         cmp #4, &sw	; s-4
	         jnc default	; doesn't borrow if s > 3

;;;  index into jt
	         mov &sw, r12
	         add r12, r12	; r12=2*s
	         mov jt(r12), r0 ; jmp jt[s]

;;;  switch table options
;;;  same order as in source code
play:		mov #1, &start
		jmp end
	
stop:		 mov #0, &start
	         jmp end	; break
	
default:	 mov #0, &start	; no break
	
end:	     pop r0          	; return
