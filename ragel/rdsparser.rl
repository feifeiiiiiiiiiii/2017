// fork https://github.com/Moodstocks/redisk

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct rds_parser_s rds_parser_t;

struct rds_parser_s {
    int arg_num;
    int arg_size;
    int cur_arg;
    int cur_arg_char;
    char **args;
    size_t *arg_sizes;
};

int rds_parser_init(rds_parser_t *sc);
int rds_parser_exec(rds_parser_t *sc, const char *data, int len);
int rds_parser_finish(rds_parser_t *sc);
int rds_parser_free(rds_parser_t *sc);

static int cs;

%%{
	machine rdsparser;
	write data;

    action args_init {
        sc->cur_arg = -1;
        sc->args = (char **)malloc(sc->arg_num * sizeof(char *));
        sc->arg_sizes = (size_t *)malloc(sc->arg_num * sizeof(size_t));
    }

    action arg_init {
        sc->cur_arg++;
        sc->cur_arg_char = 0;
        sc->arg_sizes[sc->cur_arg] = sc->arg_size;
        sc->args[sc->cur_arg] = (char *)malloc(sc->arg_size);
    }

    action test_arg_len { sc->cur_arg_char < sc->arg_size }

    action arg_add_char {
        sc->args[sc->cur_arg][sc->cur_arg_char] = fc;
        sc->cur_arg_char++;
    }

    action arg_num_add_digit { 
        sc->arg_num = sc->arg_num * 10 + (fc - '0');
    }

    action arg_size_reset { sc->arg_size = 0; }

    action arg_size_add_digit {
        sc->arg_size = sc->arg_size * 10 + (fc - '0');
    }

    rds_arg_num = '*' ( digit @arg_num_add_digit)+ '\r\n';
    rds_arg_size = '$' @arg_size_reset ( digit @arg_size_add_digit)+ '\r\n';
    rds_arg = (any when test_arg_len @arg_add_char)+ '\r\n';
    rds_cmd = rds_arg_num @args_init (rds_arg_size @arg_init rds_arg)+;

    main := rds_cmd;

}%%

int rds_parser_init(rds_parser_t *sc) {
    sc->arg_num = 0;
    sc->arg_size = 0;
    
    %% write init;

    return 1;
}

int rds_parser_exec(rds_parser_t *sc, const char *data, int len) {
    const char *p = data; // machine state use data start
    const char *pe = data + len; // machine state use data end

    %% write exec;
    if(cs == rdsparser_error) {
        return -1;
    } else if(cs >= rdsparser_first_final) {
        return 1;
    }
    return 0;
}

int rds_parser_finish(rds_parser_t *sc) {
	if (cs == rdsparser_error) return -1;
	else if (cs >= rdsparser_first_final) return 1;
	else return 0;
}

int rds_parser_free(rds_parser_t *sc) {
	int i;
	for(i=0; i<sc->arg_num; ++i) free(sc->args[i]);
	free(sc->args);
	free(sc->arg_sizes);
	return 1;
}

#define REDIS_SET_EXAMPLE "*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n"

int main(int argc, char *argv[]) {
	rds_parser_t rparser;
	rds_parser_init(&rparser);
    rds_parser_exec(&rparser, REDIS_SET_EXAMPLE, strlen(REDIS_SET_EXAMPLE));
    int finish_state = rds_parser_finish(&rparser);
    if (finish_state <= 0) {
        printf("parsing error (%d)\n", finish_state);
        rds_parser_free(&rparser);
        return 1;
    }
    else {
        printf("parsing ok (%d args)\n", rparser.arg_num);
        int i;
        for(i = 0; i < rparser.arg_num; ++i)
        printf("  %.*s\n", (int)rparser.arg_sizes[i], rparser.args[i]);
        rds_parser_free(&rparser);
        return 0;
    }
}
