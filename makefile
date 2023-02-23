APP_NAME = creed
SOURCE = token.c token.h lexer.c lexer.h main.c
TEST = test.cr

all: run

build: ${SOURCE}
	gcc ${SOURCE} -o ${APP_NAME} -Wall -Werror -std=c99

run:
	make build
	./${APP_NAME} ${TEST}


clean:
	rm -f ${APP_NAME}
