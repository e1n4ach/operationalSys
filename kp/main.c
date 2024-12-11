#include "dag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_config(const char *filename, Config *config);
int execute_dag(const Config *config);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return 1;
    }

    printf("argc=%d\n", argc);
    if (argc > 1) {
        printf("argv[1]=%s\n", argv[1]);
    }

    Config config;
    int ret = parse_config(argv[1], &config);
    if (ret != 0) {
        fprintf(stderr, "Error parsing config.\n");
        return 1;
    }

    printf("Parsed configuration:\n");
    printf("max_parallel_jobs: %d\n", config.max_parallel_jobs);
    printf("Jobs:\n");
    for (int i = 0; i < config.job_count; i++) {
        printf("  Job %s: deps=[", config.jobs[i].name);
        for (int d = 0; d < config.jobs[i].dep_count; d++) {
            printf("%s%s", config.jobs[i].deps[d], d + 1 < config.jobs[i].dep_count ? "," : "");
        }
        printf("], barrier=%s\n", config.jobs[i].barrier[0] ? config.jobs[i].barrier : "(none)");
    }

    printf("Barriers:\n");
    for (int i = 0; i < config.barrier_count; i++) {
        printf("  Barrier %s: jobs=[", config.barriers[i].name);
        for (int j = 0; j < config.barriers[i].job_count; j++) {
            printf("%s%s", config.barriers[i].jobs[j], j + 1 < config.barriers[i].job_count ? "," : "");
        }
        printf("]\n");
    }

    int res = execute_dag(&config);
    return res == 0 ? 0 : 1;
}
