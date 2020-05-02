# makfile configuration
CPU             	= msp430g2553
CFLAGS          	= -mmcu=${CPU} -Os -I../h
LDFLAGS		= -L../lib -L/opt/ti/msp430_gcc/include/ 

#switch the compiler (for the internal make rules)
CC              = msp430-elf-gcc
AS              = msp430-elf-gcc -mmcu=${CPU} -c

all:pongMain.elf

#additional rules for files
pongMain.elf: ${COMMON_OBJECTS} pongMain.o wdt_handler.o stateMachine.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^ -lTimer -lLcd -lShape -lCircle -lp2sw

load: pongMain.elf
	mspdebug rf2500 "prog $^"

clean:
	rm -f *.o *.elf *~
