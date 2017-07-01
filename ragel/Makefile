RAGEL = ragel -G2
CXX = gcc

rdsparser.c: rdsparser.rl
	${RAGEL} rdsparser.rl

rdsparser: rdsparser.c
	${CXX} rdparser.c -o rdparser

all: rdsparser.c rdsparser

clean:
	rm -r rdsparser.c rdsparser
