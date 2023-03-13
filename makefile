APP_NAME = creed
SOURCE = print.c print.h string_builder.c string_builder.h token.c token.h lexer.c lexer.h parser.c parser.h main.c
TEST = -test_parser

all: run

build: ${SOURCE}
	gcc ${SOURCE} -o ${APP_NAME} -Wall -Werror -std=c99 -lm -g

run:
	make build
	./${APP_NAME} ${TEST}


clean:
	rm -f ${APP_NAME}
