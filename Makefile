.PHONY: clean run gdb

ifeq (${DEBUG}, 1)
  LFLAGS   += --debug --trace
  CXXFLAGS += -Wall -Wextra -Wpedantic 
  CXXFLAGS += -DDEBUG -O0 -ggdb -fno-inline	
  WRAP     := valgrind --track-origins=yes --leak-check=full --show-leak-kinds=all 
else
  CXXFLAGS += -O3 -fno-stack-protector -fno-exceptions -fno-rtti
endif

LDLIBS   += -lfl $$(pkgconf --cflags --libs menu) $$(pkgconf --cflags --libs ncurses) $$(pkgconf --cflags --libs readline)
CXXFLAGS += -std=gnu++20 -I./source/ -I./object/ -I./

OBJD:=object/
SRCD:=source/
SRC:=main.cpp tui.cpp
SRC:=$(addprefix ${SRCD},${SRC}) ${OBJD}/lex.cpp 
OBJ:=$(subst .cpp,.o,$(subst ${SRCD},${OBJD},${SRC}))

OUTPUT:=rc-tui

main: lexer ${OBJ}
	${LINK.cpp} ${OBJ} -o ${OUTPUT} ${LDLIBS}

object/%.o: source/%.cpp
	${COMPILE.cpp} $< -o $@

lexer: source/rc_lexer.l
	${LEX} ${LFLAGS} --header-file=${OBJD}/rc_lexer.h -o ${OBJD}/lex.cpp ${SRCD}/rc_lexer.l

clean:
	-rm ${OBJD}/*
	-rm ./${OUTPUT}

run:
	./${OUTPUT}

gdb:
	sudo gdb --directory=./source -p $(shell pgrep ${OUTPUT})
