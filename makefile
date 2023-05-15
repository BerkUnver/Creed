APP_NAME = creed
SOURCE = prelude.c string_builder.c string_cache.c token.c lexer.c parser.c symbol_table.c handlers.c main.c

all: run

build: ${SOURCE}
	gcc ${SOURCE} -o ${APP_NAME} -Wall -Werror -pedantic -std=c99 -lm -g

run:
	make build
	./${APP_NAME}


clean:
	rm -f ${APP_NAME} file.c
