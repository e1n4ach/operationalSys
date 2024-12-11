#include "dag.h"
#include <yaml.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_config(const char *filename, Config *config);

static int expect_scalar(yaml_parser_t *parser, yaml_event_t *event) {
    if (!yaml_parser_parse(parser, event)) return 0;
    if (event->type != YAML_SCALAR_EVENT) {
        yaml_event_delete(event);
        return 0;
    }
    return 1;
}

static int skip_node(yaml_parser_t *parser) {
    yaml_event_t event;
    if (!yaml_parser_parse(parser, &event)) return 0;

    if (event.type == YAML_SCALAR_EVENT) {
        yaml_event_delete(&event);
        return 1;
    } else if (event.type == YAML_SEQUENCE_START_EVENT) {
        yaml_event_delete(&event);
        int depth = 1;
        while (depth > 0) {
            if(!yaml_parser_parse(parser, &event)) return 0;
            if (event.type == YAML_SEQUENCE_START_EVENT) depth++;
            else if (event.type == YAML_SEQUENCE_END_EVENT) depth--;
            else if (event.type == YAML_STREAM_END_EVENT) {
                yaml_event_delete(&event);
                return 0;
            }
            yaml_event_delete(&event);
        }
        return 1;
    } else if (event.type == YAML_MAPPING_START_EVENT) {
        yaml_event_delete(&event);
        int depth = 1;
        while (depth > 0) {
            if(!yaml_parser_parse(parser, &event)) return 0;
            if (event.type == YAML_MAPPING_START_EVENT) depth++;
            else if (event.type == YAML_MAPPING_END_EVENT) depth--;
            else if (event.type == YAML_STREAM_END_EVENT) {
                yaml_event_delete(&event);
                return 0;
            }
            yaml_event_delete(&event);
        }
        return 1;
    } else if (event.type == YAML_STREAM_END_EVENT) {
        yaml_event_delete(&event);
        return 0;
    } else {
        yaml_event_delete(&event);
        return 1;
    }
}

int parse_config(const char *filename, Config *config) {
    memset(config,0,sizeof(*config));

    FILE *f = fopen(filename,"rb");
    if(!f) {
        fprintf(stderr,"Cannot open config file: %s\n", filename);
        return -1;
    }

    yaml_parser_t parser;
    yaml_event_t event;

    if(!yaml_parser_initialize(&parser)) {
        fprintf(stderr,"YAML parser init failed\n");
        fclose(f);
        return -1;
    }

    yaml_parser_set_input_file(&parser,f);

    config->max_parallel_jobs=1;

    int in_root_mapping=0;

    while(1) {
        if(!yaml_parser_parse(&parser,&event)) {
            fprintf(stderr,"YAML parse error\n");
            yaml_parser_delete(&parser);
            fclose(f);
            return -1;
        }
        if(event.type == YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
            break;
        }
        if(event.type == YAML_MAPPING_START_EVENT && !in_root_mapping) {
            in_root_mapping=1;
            yaml_event_delete(&event);
            continue;
        }
        if(event.type == YAML_SCALAR_EVENT && in_root_mapping) {
            char key[MAX_NAME_LEN];
            strncpy(key,(char*)event.data.scalar.value, MAX_NAME_LEN-1);
            key[MAX_NAME_LEN-1]='\0';
            yaml_event_delete(&event);

            if(strcmp(key,"max_parallel_jobs")==0) {
                if(!yaml_parser_parse(&parser,&event)) {
                    fprintf(stderr,"Failed to parse max_parallel_jobs value\n");
                    yaml_parser_delete(&parser);
                    fclose(f);
                    return -1;
                }
                if(event.type==YAML_SCALAR_EVENT) {
                    config->max_parallel_jobs = atoi((char*)event.data.scalar.value);
                    yaml_event_delete(&event);
                } else {
                    fprintf(stderr,"max_parallel_jobs value is not scalar\n");
                    yaml_event_delete(&event);
                    yaml_parser_delete(&parser);
                    fclose(f);
                    return -1;
                }
            } else if(strcmp(key,"barriers")==0) {
                if(!yaml_parser_parse(&parser,&event)) {
                    fprintf(stderr,"Failed to parse barriers\n");
                    yaml_parser_delete(&parser);
                    fclose(f);
                    return -1;
                }
                if(event.type!=YAML_SEQUENCE_START_EVENT) {
                    fprintf(stderr,"barriers should be a sequence\n");
                    yaml_event_delete(&event);
                    yaml_parser_delete(&parser);
                    fclose(f);
                    return -1;
                }
                yaml_event_delete(&event);
                while(1) {
                    if(!yaml_parser_parse(&parser,&event)) {
                        fprintf(stderr,"Error parsing barrier sequence\n");
                        yaml_parser_delete(&parser);
                        fclose(f);
                        return -1;
                    }
                    if(event.type==YAML_SEQUENCE_END_EVENT) {
                        yaml_event_delete(&event);
                        break;
                    } else if(event.type==YAML_MAPPING_START_EVENT) {
                        if(config->barrier_count>=MAX_BARRIERS) {
                            fprintf(stderr,"Too many barriers\n");
                            yaml_event_delete(&event);
                            yaml_parser_delete(&parser);
                            fclose(f);
                            return -1;
                        }
                        BarrierInfo *b = &config->barriers[config->barrier_count];
                        memset(b,0,sizeof(*b));
                        b->passed=0;
                        yaml_event_delete(&event);
                        int done_barrier=0;
                        while(!done_barrier) {
                            if(!yaml_parser_parse(&parser,&event)) {
                                fprintf(stderr,"Error parsing barrier fields\n");
                                yaml_parser_delete(&parser);
                                fclose(f);
                                return -1;
                            }
                            if(event.type==YAML_MAPPING_END_EVENT) {
                                done_barrier=1;
                                yaml_event_delete(&event);
                                break;
                            }
                            if(event.type==YAML_SCALAR_EVENT) {
                                char bk[MAX_NAME_LEN];
                                strncpy(bk,(char*)event.data.scalar.value,MAX_NAME_LEN-1);
                                bk[MAX_NAME_LEN-1]='\0';
                                yaml_event_delete(&event);

                                if(strcmp(bk,"name")==0) {
                                    if(!expect_scalar(&parser,&event)) {
                                        fprintf(stderr,"barrier name missing\n");
                                        yaml_parser_delete(&parser);
                                        fclose(f);
                                        return -1;
                                    }
                                    strncpy(b->name,(char*)event.data.scalar.value,MAX_NAME_LEN-1);
                                    b->name[MAX_NAME_LEN-1] = '\0';
                                    yaml_event_delete(&event);
                                } else if(strcmp(bk,"jobs")==0) {
                                    if(!yaml_parser_parse(&parser,&event)) {
                                        fprintf(stderr,"error parsing barrier jobs\n");
                                        yaml_parser_delete(&parser);
                                        fclose(f);
                                        return -1;
                                    }
                                    if(event.type!=YAML_SEQUENCE_START_EVENT) {
                                        fprintf(stderr,"barrier jobs should be a sequence\n");
                                        yaml_event_delete(&event);
                                        yaml_parser_delete(&parser);
                                        fclose(f);
                                        return -1;
                                    }
                                    yaml_event_delete(&event);
                                    int jcount=0;
                                    while(1) {
                                        if(!yaml_parser_parse(&parser,&event)) {
                                            fprintf(stderr,"error parsing barrier job list\n");
                                            yaml_parser_delete(&parser);
                                            fclose(f);
                                            return -1;
                                        }
                                        if(event.type==YAML_SEQUENCE_END_EVENT) {
                                            yaml_event_delete(&event);
                                            break;
                                        }
                                        if(event.type==YAML_SCALAR_EVENT) {
                                            if(jcount>=MAX_DEPS) {
                                                fprintf(stderr,"Too many jobs in barrier\n");
                                                yaml_event_delete(&event);
                                                yaml_parser_delete(&parser);
                                                fclose(f);
                                                return -1;
                                            }
                                            strncpy(b->jobs[jcount],(char*)event.data.scalar.value,MAX_NAME_LEN-1);
                                            b->jobs[jcount][MAX_NAME_LEN-1]='\0';
                                            jcount++;
                                            yaml_event_delete(&event);
                                        } else {
                                            yaml_event_delete(&event);
                                        }
                                    }
                                    b->job_count=jcount;
                                    b->remaining=jcount;
                                } else {
                                    if(!skip_node(&parser)) {
                                        fprintf(stderr,"Error skipping unknown barrier key\n");
                                        yaml_parser_delete(&parser);
                                        fclose(f);
                                        return -1;
                                    }
                                }
                            } else {
                                yaml_event_delete(&event);
                            }
                        }
                        config->barrier_count++;
                    } else {
                        yaml_event_delete(&event);
                    }
                }
            } else if(strcmp(key,"jobs")==0) {
                if(!yaml_parser_parse(&parser,&event)) {
                    fprintf(stderr,"Failed to parse jobs\n");
                    yaml_parser_delete(&parser);
                    fclose(f);
                    return -1;
                }
                if(event.type!=YAML_SEQUENCE_START_EVENT) {
                    fprintf(stderr,"jobs should be a sequence\n");
                    yaml_event_delete(&event);
                    yaml_parser_delete(&parser);
                    fclose(f);
                    return -1;
                }
                yaml_event_delete(&event);
                while(1) {
                    if(!yaml_parser_parse(&parser,&event)) {
                        fprintf(stderr,"Error parsing job sequence\n");
                        yaml_parser_delete(&parser);
                        fclose(f);
                        return -1;
                    }
                    if(event.type==YAML_SEQUENCE_END_EVENT) {
                        yaml_event_delete(&event);
                        break;
                    } else if(event.type==YAML_MAPPING_START_EVENT) {
                        if(config->job_count>=MAX_JOBS) {
                            fprintf(stderr,"Too many jobs\n");
                            yaml_event_delete(&event);
                            yaml_parser_delete(&parser);
                            fclose(f);
                            return -1;
                        }
                        Job *job=&config->jobs[config->job_count];
                        memset(job,0,sizeof(*job));
                        yaml_event_delete(&event);
                        int done_job=0;
                        while(!done_job) {
                            if(!yaml_parser_parse(&parser,&event)) {
                                fprintf(stderr,"Error parsing job fields\n");
                                yaml_parser_delete(&parser);
                                fclose(f);
                                return -1;
                            }
                            if(event.type==YAML_MAPPING_END_EVENT) {
                                done_job=1;
                                yaml_event_delete(&event);
                                break;
                            }
                            if(event.type==YAML_SCALAR_EVENT) {
                                char jk[MAX_NAME_LEN];
                                strncpy(jk,(char*)event.data.scalar.value,MAX_NAME_LEN-1);
                                jk[MAX_NAME_LEN-1]='\0';
                                yaml_event_delete(&event);
                                if(strcmp(jk,"name")==0) {
                                    if(!expect_scalar(&parser,&event)) {
                                        fprintf(stderr,"job name missing\n");
                                        yaml_parser_delete(&parser);
                                        fclose(f);
                                        return -1;
                                    }
                                    strncpy(job->name,(char*)event.data.scalar.value,MAX_NAME_LEN-1);
                                    job->name[MAX_NAME_LEN-1] = '\0';
                                    yaml_event_delete(&event);
                                } else if(strcmp(jk,"deps")==0) {
                                    if(!yaml_parser_parse(&parser,&event)) {
                                        fprintf(stderr,"error parsing job deps\n");
                                        yaml_parser_delete(&parser);
                                        fclose(f);
                                        return -1;
                                    }
                                    if(event.type!=YAML_SEQUENCE_START_EVENT) {
                                        fprintf(stderr,"deps should be a sequence\n");
                                        yaml_event_delete(&event);
                                        yaml_parser_delete(&parser);
                                        fclose(f);
                                        return -1;
                                    }
                                    yaml_event_delete(&event);
                                    int dep_i=0;
                                    while(1) {
                                        if(!yaml_parser_parse(&parser,&event)) {
                                            fprintf(stderr,"error parsing deps list\n");
                                            yaml_parser_delete(&parser);
                                            fclose(f);
                                            return -1;
                                        }
                                        if(event.type==YAML_SEQUENCE_END_EVENT) {
                                            yaml_event_delete(&event);
                                            break;
                                        }
                                        if(event.type==YAML_SCALAR_EVENT) {
                                            if(dep_i>=MAX_DEPS) {
                                                fprintf(stderr,"Too many dependencies\n");
                                                yaml_event_delete(&event);
                                                yaml_parser_delete(&parser);
                                                fclose(f);
                                                return -1;
                                            }
                                            strncpy(job->deps[dep_i],(char*)event.data.scalar.value,MAX_NAME_LEN-1);
                                            job->deps[dep_i][MAX_NAME_LEN-1] = '\0';
                                            dep_i++;
                                            yaml_event_delete(&event);
                                        } else {
                                            yaml_event_delete(&event);
                                        }
                                    }
                                    job->dep_count=dep_i;
                                } else if(strcmp(jk,"barrier")==0) {
                                    if(!expect_scalar(&parser,&event)) {
                                        fprintf(stderr,"barrier name missing\n");
                                        yaml_parser_delete(&parser);
                                        fclose(f);
                                        return -1;
                                    }
                                    strncpy(job->barrier,(char*)event.data.scalar.value,MAX_NAME_LEN-1);
                                    job->barrier[MAX_NAME_LEN-1] = '\0';
                                    yaml_event_delete(&event);
                                } else {
                                    if(!skip_node(&parser)) {
                                        fprintf(stderr,"Error skipping unknown job key\n");
                                        yaml_parser_delete(&parser);
                                        fclose(f);
                                        return -1;
                                    }
                                }
                            } else {
                                yaml_event_delete(&event);
                            }
                        }
                        config->job_count++;
                    } else {
                        yaml_event_delete(&event);
                    }
                }
            } else {
                if(!skip_node(&parser)) {
                    fprintf(stderr,"Error skipping unknown top-level key\n");
                    yaml_parser_delete(&parser);
                    fclose(f);
                    return -1;
                }
            }
        } else {
            yaml_event_delete(&event);
        }
    }

    yaml_parser_delete(&parser);
    fclose(f);

    for (int i=0; i<config->job_count; i++) {
        for (int d=0; d<config->jobs[i].dep_count; d++) {
            int found=0;
            for(int j=0;j<config->job_count;j++) {
                if(strcmp(config->jobs[j].name, config->jobs[i].deps[d])==0) {
                    found=1;
                    break;
                }
            }
            if(!found) {
                fprintf(stderr,"Error: job '%s' depends on unknown job '%s'\n",
                        config->jobs[i].name, config->jobs[i].deps[d]);
                return -1;
            }
        }
        if(strlen(config->jobs[i].barrier)>0) {
            int found=0;
            for(int b=0;b<config->barrier_count;b++) {
                if(strcmp(config->barriers[b].name,config->jobs[i].barrier)==0) {
                    found=1;
                    break;
                }
            }
            if(!found) {
                fprintf(stderr,"Error: job '%s' references unknown barrier '%s'\n",
                        config->jobs[i].name, config->jobs[i].barrier);
                return -1;
            }
        }
    }

    return 0;
}
