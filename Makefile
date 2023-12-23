.PHONY: main clean run

ifeq (${DEBUG}, 1)
  LFLAGS   += --debug --trace
  CXXFLAGS += -Wall -Wextra -Wpedantic 
  CXXFLAGS += -DDEBUG -O0 -ggdb -fno-inline	
  WRAP     := valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all 
else
  CXXFLAGS += -O3 -fno-stack-protector -fno-exceptions -fno-rtti
endif

# XXX:
COMP:=$(CXX) $(CFXXLAGS) $(LDFLAGS) $(LDLIBS)

OBJD:=object/
SRCD:=source/
SRC:=main.cpp lex.cpp tui.cpp
SRC:=$(addprefix ${SRCD},${SRC})
OBJ:=$(subst .cpp,.o,$(subst ${SRCD},${OBJD},${SRC}))

OUTPUT:=main.out

main: lexer ${OBJ}
	${COMP} ${OBJ} -o ${OUTPUT} -lfl `pkgconf --libs menu` `pkgconf --libs ncurses`

object/%.o: source/%.cpp
	${COMP} -c $< -o ${OBJD}/$*.o

lexer:
	${LEX} ${LFLAGS} --header-file=${SRCD}/rc_lexer.h -o ${SRCD}/lex.cpp ${SRCD}/rc_lexer.l

clean:
	-rm ${OBJD}/*
	-rm ${SRCD}/lex.yy.c
	-rm ./${OUTPUT}

run: main
	./${OUTPUT}

gdb:
	sudo gdb --directory=./source -p $(shell pgrep ${OUTPUT})
