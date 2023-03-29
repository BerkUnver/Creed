APP_NAME = creed
SOURCE = print.c string_builder.c string_cache.c token.c lexer.c parser.c main.c

all: run

build: ${SOURCE}
	gcc ${SOURCE} -o ${APP_NAME} -Wall -Werror -std=c99 -lm -g

run:
	make build
	./${APP_NAME}


clean:
	rm -f ${APP_NAME}
