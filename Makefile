CXX:=g++
CFLAGS:=-O0 -ggdb -std=c++17
LDLIBS:=
LDFLAGS:=
COMP:=$(CXX) $(CFLAGS) $(LDFLAGS) $(LDLIBS)

OBJD:=obj/
SRCD:=src/
SRC:=main.cpp lex.cpp tui.cpp
SRC:=$(addprefix ${SRCD},${SRC})
OBJ:=$(subst .cpp,.o,$(subst ${SRCD},${OBJD},${SRC}))

OUTPUT:=main.out

.PHONY: main clean run

main: lexer ${OBJ}
	${COMP} ${OBJ} -o ${OUTPUT} -lfl `pkgconf --libs ncurses`

obj/%.o: src/%.cpp
	${COMP} -c $< -o ${OBJD}/$*.o

lexer:
	flex --header-file=${SRCD}/rc_lexer.h -o ${SRCD}/lex.cpp ${SRCD}/rc_lexer.l

clean:
	rm ${OBJD}/*
	rm ${SRCD}/lex.yy.c
	rm ./${OUTPUT}

run:
	./${OUTPUT}

gdb:
	sudo gdb --directory=./source -p $(shell pgrep ${OUTPUT})
