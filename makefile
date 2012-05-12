#
# Frikqcc makefile
#


PROG=		frikqcc
CDEFS=		-DINLINE 
CWARN=		-Wall -W
CFLAGS= 	-O2 $(CWARN) $(CDEFS)
OBJFILES=	cmdlib.o pr_comp.o pr_lex.o qcc.o hash.o main.o

all:	$(PROG)

$(PROG): $(OBJFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROG) $(OBJFILES) $(LIBS)

clean:
	rm -rf $(PROG) $(OBJFILES) *.bak *~ make.log error.log
