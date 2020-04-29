/*
 * Copyright (c) 2020 Fingerprint Cards AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * 04/14/2020: Added SPI interfcae support
 */


/**
 * @file    main.c
 * @brief   Main file for FPC BM-Lite Communication example.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

#include "bep_host_if.h"
#include "com_common.h"
#include "platform.h"

static void help(void)
{
    fprintf(stderr, "BEP Host Communication Application\n");
    fprintf(stderr, "Syntax: bep_host_com [-s] [-p port] [-b baudrate] [-t timeout]\n");
}

int main (int argc, char **argv)
{
    char *port = NULL;
    int baudrate = 921600;
    int timeout = 5;
    int index;
    int c;
    uint8_t buffer[512];
    uint16_t size[2] = { 256, 256 };
    fpc_com_chain_t hcp_chain;
    interface_t iface = COM_INTERFACE;

    opterr = 0;

    while ((c = getopt (argc, argv, "sb:p:t:")) != -1) {
        switch (c) {
            case 's':
                iface = SPI_INTERFACE;
                if(baudrate == 921600)
                    baudrate = 1000000;
                break;
            case 'b':
                baudrate = atoi(optarg);
                break;
            case 'p':
                port = optarg;
                break;
            case 't':
                timeout = atoi(optarg);
                break;
            case '?':
                if (optopt == 'b')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                return 1;
            default:
                help();
                exit(1);
            }
        }

    if (iface == COM_INTERFACE && port == NULL) {
        printf("port must be specified\n");
        help();
        exit(1);
    }

    for (index = optind; index < argc; index++) {
        printf ("Non-option argument %s\n", argv[index]);
    }

    switch (iface) {
        case SPI_INTERFACE:
            if(!platform_spi_init(baudrate)) {
                printf("SPI initialization failed\n");
                exit(1);
            }
            break;
        case COM_INTERFACE:
            if (!platform_com_init(port, baudrate, timeout)) {
                printf("Com initialization failed\n");
                exit(1);
            }
            break;
        default:
            printf("Interface not specified'n");
            help();
            exit(1);
    }

    init_com_chain(&hcp_chain, buffer, size, NULL, iface);
    hcp_chain.channel = 1;

    while(1) {
        char cmd[100];
        fpc_bep_result_t res = FPC_BEP_RESULT_OK;
        uint16_t template_id;
        bool match;

        platform_clear_screen();
        printf("BM-Lite Interface\n");
        if (iface == SPI_INTERFACE)
        	printf("SPI port: speed %d Hz\n", baudrate);
        else
            printf("Com port: %s [speed: %d]\n", port, baudrate);
        printf("Timeout: %ds\n", timeout);
        printf("-------------------\n\n");
        printf("Possible options:\n");
        printf("a: Enroll finger\n");
        printf("b: Capture and identify finger\n");
        printf("c: Remove all templates\n");
        printf("d: Save template\n");
        printf("e: Remove template\n");
        printf("t: Get template\n");
        printf("T: Put template\n");
        printf("f: Capture image\n");
        printf("g: Image upload\n");
        printf("h: Get version\n");
        printf("q: Exit program\n");
        printf("\nOption>> ");
        fgets(cmd, sizeof(cmd), stdin);
        switch (cmd[0]) {
            case 'a':
                res = bep_enroll_finger(&hcp_chain);
                break;
            case 'b':
                res = bep_identify_finger(&hcp_chain, &template_id, &match);
                if (res == FPC_BEP_RESULT_OK) {
                    if (match) {
                        printf("Match with template id: %d\n", template_id);
                    } else {
                        printf("No match\n");
                    }
                }
                break;
            case 'c':
                res = bep_delete_template(&hcp_chain, REMOVE_ID_ALL_TEMPLATES);
                break;
            case 'd':
                printf("Template id: ");
                fgets(cmd, sizeof(cmd), stdin);
                template_id = atoi(cmd);
                res = bep_save_template(&hcp_chain, template_id);
                break;
            case 't': {
                printf("Template id: ");
                fgets(cmd, sizeof(cmd), stdin);
                template_id = atoi(cmd);
                uint8_t *buf = malloc(10240);
                memset(buf,0,10240);
                uint16_t tmpl_size = 0;
                res = bep_template_get(&hcp_chain, template_id, buf, 10240, &tmpl_size);
                printf("Template size: %d\n", tmpl_size);
                if (res == FPC_BEP_RESULT_OK) {
                    FILE *f = fopen("tmpl.raw", "wb");
                    if (f) {
                        fwrite(buf, tmpl_size, 1, f);
                        fclose(f);
                        printf("Template saved as tmpl.raw\n");
                    }
                }
                free(buf);
                break;
            }
            case 'T': {
                printf("Template id: ");
                fgets(cmd, sizeof(cmd), stdin);
                template_id = atoi(cmd);
                uint8_t *buf = malloc(10240);
                memset(buf,0,10240);
                uint16_t tmpl_size = 0;
                FILE *f = fopen("tmpl.raw", "rb");
                if (f) {
                    printf("Read template from tmpl.raw\n");
                    tmpl_size = fread(buf, 1, 10240, f);
                    printf("Template size: %d\n", tmpl_size);
                    fclose(f);
                }
                res = bep_template_put(&hcp_chain, template_id, buf, tmpl_size);
                free(buf);
                break;
                }
            case 'e':
                printf("Template id: ");
                fgets(cmd, sizeof(cmd), stdin);
                template_id = atoi(cmd);
                res = bep_delete_template(&hcp_chain, template_id);
                break;
            case 'f':
                printf("Timeout (ms): ");
                fgets(cmd, sizeof(cmd), stdin);
                res = bep_capture(&hcp_chain, atoi(cmd));
                break;
            case 'g': {
                uint32_t size;
                res = bep_image_get_size(&hcp_chain, &size);
                if (res == FPC_BEP_RESULT_OK) {
                    uint8_t *buf = malloc(size);
                    if (buf) {
                      res = bep_image_get(&hcp_chain, buf, size);
                      if (res == FPC_BEP_RESULT_OK) {
                          FILE *f = fopen("image.raw", "wb");
                          if (f) {
                              fwrite(buf, 1, size, f);
                              fclose(f);
                              printf("Image saved as image.raw\n");
                          }
                      }
                    }

                }
                break;
            }
            case 'h': {
                char version[100];

                memset(version, 0, 100);
                res = bep_version(&hcp_chain, version, 99);
                if (res == FPC_BEP_RESULT_OK) {
                  printf("%s\n", version);
                }
                break;
            }
            case 'q':
                return 0;
            default:
                printf("\nUnknown command\n");
        }
        if (res == FPC_BEP_RESULT_OK) {
            printf("\nCommand succeded\n");
        } else {
            printf("\nCommand failed with error code %d\n", res);
        }
        printf("Press any key to continue...");
        fgets(cmd, sizeof(cmd), stdin);
    }

    return 0;
}
