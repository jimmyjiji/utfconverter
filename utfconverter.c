#include "utfconverter.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>

bool outUTF16LE = false;
bool outUTF16BE = false;
bool outUTF8= false;
bool inUTF8 = false;
bool inUTF16LE = false;
bool inUTF16BE = false;
int v = 0;
char *input_path = NULL;
char *output_path = NULL;
char hostname[8];
bool sparky = false;

#ifdef CSE320
    #define info(info, msg) printf("CSE320: %s: %s\n", info, msg)
    #define input(info, name, inode, device, size) printf ("CSE320: %s: %s %d %d %d bytes\n", info, name, inode, device, size)
#else
    #define info(info, msg) 
    #define input(info, name, inode, device, size)
#endif

int main(int argc, char* argv[]) {
    int opt = EXIT_FAILURE;
    bool hasE = false;
    char *encodingOut = NULL;
    
    hostname[7] = '\0';
    gethostname(hostname, 8);
    /* open output channel */
    if (strcmp(hostname, "sparky") == 0)
        sparky = true;


    while (((opt = getopt(argc, argv, "veh")) != -1)) {
        switch(opt) {
            case 'v':
                v++;
                break;
            case 'e':
                encodingOut = argv[optind++];
                hasE = true;
                break;
            case 'h':
                /* The help menu was selected */
                USAGE(argv[0]);
                exit(EXIT_SUCCESS);
        }
    }

    if (!hasE) {
            USAGE(argv[0]);
            fprintf(stderr, "%s\n", "Missing the `-e` flag. This flag is required.\n The output file was not created");
            exit(EXIT_FAILURE);
        }


    if (strcmp(encodingOut, "UTF-8")==0) {
        outUTF8 = true;
    } else if (strcmp(encodingOut, "UTF-16LE") == 0) {
        outUTF16LE = true;
    } else if (strcmp(encodingOut, "UTF-16BE") == 0) {
        outUTF16BE = true;
    } else {
        fprintf(stderr, "%s\n","The flag `-e` has an incorrect value `BAD`. The output file was not created." );
        exit(EXIT_FAILURE);
    }

    /* Get position arguments */
    if(argc >= 4) {
        input_path = argv[argc-2];
        output_path = argv[argc-1];
        if (strstr(input_path, ".txt") == NULL || strstr(output_path, ".txt") == NULL) {
            fprintf(stderr, "Invalid input. Please refer to usage statement '-h'.\n");
            USAGE(0[argv]);
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Invalid input. Please refer to usage statement '-h'.\n");
        USAGE(0[argv]);
        exit(EXIT_FAILURE);
    }

    /* Make sure all the arguments were provided */
    if(input_path != NULL || output_path != NULL) {
        int input_fd = -1;
        int output_fd = -1;
        bool success = false;
        switch(validate_args(input_path, output_path)) {
            case VALID_ARGS:
                    /* Attempt to open the input file */
            if((input_fd = open(input_path, O_RDONLY)) < 0) {
                fprintf(stderr, "Failed to open the file %s\n", input_path);
                goto conversion_done;
            }
                    /* Delete the output file if it exists; Don't care about return code. */
            
                    /* Attempt to create the file */
            output_fd = open(output_path, O_CREAT | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

            if(output_fd < 0) {
                        /* Tell the user that the file failed to be created */
                fprintf(stderr, "Failed to open the file %s\n", input_path);
                goto conversion_done;
            }
            


                    /* Start the conversion */
            success = convert(input_fd, output_fd);
conversion_done:    

            if(success) {
                int return_code = EXIT_SUCCESS;
                fprintf(stdout, "The output file %s was successfully created\n", output_path);
                return return_code;
            } else {
                        /* Conversion failed; clean up */
                if(output_fd < 0 && input_fd >= 0) {
                    close(input_fd);
                }
                if(output_fd >= 0) {
                    close(output_fd);
                }
                        /* Just being pedantic... */
                int return_code = EXIT_FAILURE;
                return return_code;
            }
            case SAME_FILE:
            fprintf(stderr, "The output file %s was not created. Same as input file.\n", output_path);
            break;
            case FILE_DNEI:
            fprintf(stderr, "The input file %s does not exist.\n", input_path);
            break;
            case FILE_DNEO:
            fprintf(stderr, "The output file %s does not exist \n", output_path);
            break;
            default:
            fprintf(stderr, "An unknown error occurred\n");
            break;
        }
    } else {
        /* Alert the user*/// what was not set before quitting. */
        if((input_path = NULL) == NULL) {
            fprintf(stderr, "INPUT_FILE was not set.\n");
        }
        if((output_path = NULL) == NULL) {
            fprintf(stderr, "OUTPUT_FILE was not set.\n");
        }
        // Print out the program usage
        USAGE(argv[0]);
    }
    return EXIT_FAILURE;
}

int validate_args(const char* input_path, const char* output_path) {
    int return_code = FAILED;
    /* Make sure both strings are not NULL */
    if(input_path != NULL && output_path != NULL) {
        /* Check to see if the the input and output are two different files. */
        if(strcmp(input_path, output_path) != 0) {
            /* Check to see if the input file exists */
            struct stat sb;
            struct stat sout;
            stat(input_path, &sb);
            stat(output_path, &sout);

            int inode_in = sb.st_ino;
            int inode_out = sout.st_ino;

            int devicein = sb.st_dev;
            int deviceout = sout.st_dev;

            /* zero out the memory*/
            memset(&sb, 0, sizeof(sb));
            /* now check to see if the file exists */
            if(stat(input_path, &sb) == -1 || inode_in == inode_out) {
                /* something went wrong */
                if (devicein != deviceout) {
                    if (devicein <= 0)
                        return_code = FILE_DNEI;
                    if (deviceout <= 0)
                        return_code = FILE_DNEO;
                    return return_code;
                } else if (inode_in == inode_out) {
                    return_code = SAME_FILE;
                } else if(errno == ENOENT) {
                    /* File does not exist. */
                    return_code = FILE_DNEI;
                } else {
                    /* No idea what the error is. */
                    perror("NULL");
                }
            } else {
                return_code = VALID_ARGS;
            }
        }
    }
    return return_code;
}
/* As of now, you need to make sure the first while loop gets the approrpriate byte values. T
You might need to make two loops, one to loop the whole thing and one to get 1-4 bytes at a time*/
bool convert(const int input_fd, const int output_fd) {
    bool success = false;
    if(input_fd >= 0 && output_fd >= 0) {
        unsigned int read_value_int = 0;
        auto unsigned int utf8last = 0xBF;
        auto unsigned int utf8first2 = 0xBBEF;
        auto unsigned int utf8 = 0xBFBBEF;  
        auto unsigned int utf16le = 0xFEFF;
        auto unsigned int utf16be = 0xFFFE;

       
        if (read(input_fd, &read_value_int, 2) == 2) {
            if (sparky) {
               read_value_int = fliptoBEU(read_value_int);
            }
            if (read_value_int == utf8first2) {
                unsigned int follow = 0;
               read(input_fd, &follow, 1);
               if (sparky) {
                follow = fliptoBEU(follow);
               }
               if (follow == utf8last) {
                inUTF8 = true;
               }
            } else if (read_value_int == utf16le) {
                inUTF16LE = true;
            } else if (read_value_int == utf16be) {
                inUTF16BE = true;
            } else {
                fprintf(stderr, "%s\n","This file does not have a valid encoding \n The output file wasn't created");
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr, "%s\n","Unable to read first 3 bytes");
            exit(EXIT_FAILURE);
        }

        if (outUTF16LE) {
            if (sparky) {
                utf16le = fliptoBEU(utf16le);
            }
            if(!safe_write(output_fd, &utf16le, CODE_UNIT_SIZE)) {
               fprintf(stderr, "%s\n", "Something went wrong with writing to file");
               exit(EXIT_FAILURE);
            } 
            if (inUTF16LE && outUTF16LE) {
                success = copyBytes(input_fd, output_fd, 2);
                return success;
            }
        } else if (outUTF16BE) {
            if (sparky) {
                utf16be = fliptoBEU(utf16be);
            }
            if(!safe_write(output_fd, &utf16be, CODE_UNIT_SIZE)) {
                fprintf(stderr, "%s\n", "Something went wrong with writing to file");
                exit(EXIT_FAILURE);
            } 
            if (inUTF16BE && outUTF16BE) {
                success = copyBytes(input_fd, output_fd, 2);
                return success;
            }
        } else if (outUTF8) {
            if (sparky) {
                utf8 = fliptoBEU(utf8);
            }
            if(!safe_write(output_fd, &utf8, 3)) {
                fprintf(stderr, "%s\n", "Something went wrong with writing to file");
                exit(EXIT_FAILURE);
            } 
            if (inUTF8 && outUTF8) {
                success = copyBytes(input_fd, output_fd, 3);
                return success;
            }
        } else {
            fprintf(stderr, "%s\n", "No valid out encoding");
            exit(EXIT_FAILURE);
        }
       
        if (inUTF8 && (outUTF16LE || outUTF16BE)) 
            success = utf8to16(input_fd, output_fd);
        else
            success = utf16to8(input_fd, output_fd);
    } else {
        fprintf(stderr, "%s\n","Invalid files");
        success =false;
    }


    information();
        
    return success;
}

bool utf8to16(const int input_fd, const int output_fd) {
    bool success = false;
    unsigned char bytes[4];
    auto unsigned int read_value = 0;
    auto size_t count = 0;
    auto int safe_param = SAFE_PARAM;// DO NOT DELETE, PROGRAM WILL BE UNSAFE //
    void* saftey_ptr = &safe_param;
    auto ssize_t bytes_read;
    bool encode = false;
    /* Read in UTF-8 Bytes */
    verboseTop(v);
    while((bytes_read = read(input_fd, &read_value, 1)) == 1) {
        if (sparky) {
            read_value = fliptoBEU(read_value);
        }
            /* Mask the most significant bit of the byte */
        
        unsigned char masked_value = read_value & 0x80;
        if(masked_value == 0x80) {
            if((read_value & UTF8_4_TEST) == UTF8_4_BYTE) {
                    // Check to see which byte we have encountered
                if (count == 000) {
                    int i = 0;
                    for (i = 0; i < 4; i++) {
                        if (i == 0) {
                            bytes[count++] = read_value;
                        } else if((read_value & UTF8_CONT) == UTF8_CONT) {
                                /* continuation byte */
                            bytes[count++] = read_value;
                        }  else {
                                /* Set the file position back 1 byte */
                           if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                                 /*Unsafe action! Increment! */
                            safe_param = *(int*)++saftey_ptr;
                                /* failed to move the file pointer back */
                            perror("NULL");
                            goto conversion_finish;
                        }
                    }
                    if (i != 3) {
						read_value = 0;
                        read(input_fd, &read_value, 1);
                        if (sparky) {
                            read_value = fliptoBEU(read_value);
                        }
                    }
                }
            }


        } else if ((read_value & UTF8_3_TEST) == UTF8_3_BYTE){
                    // Check to see which byte we have encountered
                   // Check to see which byte we have encountered
            if (count == 000) {
                int i = 0;
                for (i = 0; i < 3; i++) {
                    if (i == 0) {
                        bytes[count++] = read_value;
                    } else if((read_value & UTF8_CONT) == UTF8_CONT) {
                                /* continuation byte */
                        bytes[count++] = read_value;
                    }  else {
                                /* Set the file position back 1 byte */
                        if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                                /*Unsafe action! Increment! */
                            safe_param = *(int*)++saftey_ptr;
                                /* failed to move the file pointer back */
                            perror("NULL");
                            goto conversion_finish;
                        }
                    }
                    if (i != 2) {
						read_value = 0;
                        read(input_fd, &read_value, 1);
                        if (sparky) {
                            read_value = fliptoBEU(read_value);
                        }
                    }
                }
            } 
        } else if ((read_value & UTF8_2_TEST) == UTF8_2_BYTE) {
                     // Check to see which byte we have encountered
            if (count == 000) {
                int i = 0;
                for (i = 0; i < 2; i++) {
                    if (i == 0) {
                        bytes[count++] = read_value;
                    } else if((read_value & UTF8_CONT) == UTF8_CONT) {
                                /* continuation byte */
                        bytes[count++] = read_value;
                    }  else {
                                /* Set the file position back 1 byte */
                       if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                                 /*Unsafe action! Increment! */
                        safe_param = *(int*)++saftey_ptr;
                                /* failed to move the file pointer back */
                        perror("NULL");
                        goto conversion_finish;
                    }
                }
                if (i != 1){
					read_value = 0;
                    read(input_fd, &read_value, 1);
                    if (sparky) {
                        read_value = fliptoBEU(read_value);
                     }
                }
            }
        } 
    } 
            /* Encode the current values into UTF-16LE */
            encode = true;
            /*This is if the masked bit isn't 1 or ASCII*/
        } else {
            if(count == 000) {
                            /* US-ASCII */
                bytes[count++] = read_value;
                encode = true;
            } else {
                    /* Found an ASCII character but theres other characters
                     * in the buffer already.
                     * Set the file position back 1 byte.
                     */
                     if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                        /*Unsafe action! Increment! */
                        safe_param = *(int*) ++saftey_ptr;
                        /* failed to move the file pointer back */
                        perror("NULL");
                        goto conversion_finish;
                    }
                    /* Encode the current values into UTF-16LE */
                    encode = true;
                }
            }
            /* If its time to encode do it here */
            if(encode) {
                unsigned int value = 0;
                unsigned int input_value = 0;
                unsigned int output_value = 0;
                unsigned int codepoint = 0;
                bool isAscii = false;
                int i = 0;
                for(i = 0; i < count; i++) {
                    if(i == 0) {
                        if((bytes[i] & UTF8_4_TEST) == UTF8_4_BYTE) {
                            value = bytes[i] & 0x7;
                        } else if((bytes[i] & UTF8_3_TEST) == UTF8_3_BYTE) {
                            value =  bytes[i] & 0xF;
                        } else if((bytes[i] & UTF8_2_TEST) == UTF8_2_BYTE) {
                            value =  bytes[i] & 0x1F;
                        } else if((bytes[i] & 0x80) == 0) {
                            /* Value is an ASCII character */
                            value = bytes[i];
                            isAscii = true;
                        } else {
                            /* Marker byte is incorrect */
                            goto conversion_finish;
                        }
                    } else {
                        if(!isAscii) {
                            value = (value << 6) | (bytes[i] & 0x3F);
                        } 
                    }
                }
                int j = 0;
                for (j = 0 ; j < count; j++) {
                    if (j != 0)
                        input_value = (input_value << 8) | bytes[j];
                    else
                        input_value = bytes[j];
                }

                
                    /* Handle the value if its a surrogate pair*/
                if(value >= SURROGATE_PAIR && (outUTF16BE || outUTF16LE)) {
                    unsigned int vprime = value - SURROGATE_PAIR;
                    unsigned int w1 = (vprime >> 10) + 0xD800;
                    unsigned int w2 = (vprime & 0x3FF) + 0xDC00;
                    codepoint = value;
                     /*(vprime & 0x3FF) + 0xDC00*/;
                    /* write the surrogate pair to file */

                    if (outUTF16BE) {
                         unsigned int* w1pt = &w1;
                         unsigned int* w2pt = &w2;
                         flipBitsU(w1pt);
                         flipBitsU(w2pt);
                    } 
                    value = (w1 << 16) | w2;

					output_value = value;
                    if (sparky) {
                        value = fliptoBEU(value);
                        w1 = fliptoBEU(w1);
                        w2 = fliptoBEU(w2);
                    }

                 if(!safe_write(output_fd, &w1, CODE_UNIT_SIZE)) {
                    fprintf(stderr, "%s\n", "Something went wrong with writing to file");
                    exit(EXIT_FAILURE);
                }
                if(!safe_write(output_fd, &w2, CODE_UNIT_SIZE)) {
                    fprintf(stderr, "%s\n", "Something went wrong with writing to file");
                    exit(EXIT_FAILURE);
                }

            } else {
				codepoint = value;
                if (sparky) {
                    value = fliptoBEU(value);
					
                }
                unsigned int* output = &value;
                

                output_value = value;
                unsigned int* outputPt = &output_value;
                
                if (outUTF16BE) {
                    if (sparky)
                        flipBitsBEU(output);
                    else
                        flipBitsU(output);
                    
                }

                if (outUTF16LE) {
                    if (sparky)
                        flipBitsBEU(outputPt);
                    else
                        flipBitsU(outputPt);
                }

                if(!safe_write(output_fd, &value, CODE_UNIT_SIZE)) {
                    fprintf(stderr, "%s\n", "Something went wrong with writing to file");
                    exit(EXIT_FAILURE);
                } 

            }

           
            verbose(v, input_value, count, codepoint, input_value, output_value); 
                /* Done encoding the value to UTF-16LE */
            encode = false;
            count = 0;
            memset(bytes, 0, 4);
			read_value = 0;
        }
    }
        /* If we got here the operation was a success! */
    success = true;
    conversion_finish:
    return success;

}

bool utf16to8(const int input_fd, const int output_fd) {
    bool success = false;
    unsigned int bytes[4] = {0};
    auto unsigned int read_value = 0;
    unsigned int* read_value_pt = &read_value;
    auto size_t count = 0;
    auto int safe_param = SAFE_PARAM;// DO NOT DELETE, PROGRAM WILL BE UNSAFE //
    void* saftey_ptr = &safe_param;
    auto ssize_t bytes_read;
    bool encode = false;
    unsigned int surrogatechecker = 0;
    unsigned int output_value = 0;
    unsigned int input_value = 0;
	unsigned int codepoint = 0;
    verboseTop(v);
    /* Read in UTF-8 Bytes */
    while((bytes_read = read(input_fd, &read_value, 2)) == 2) {
		if (sparky) {
			read_value = fliptoBEU(read_value);
		}
		
        if (inUTF16BE) {
			if (sparky) 
				flipBitsBEU(read_value_pt);
			else
				flipBitsU(read_value_pt);
        }    

		
        /*Obtain number of bytes UTF8 is */
       if (read_value > 0x0 && read_value < 0x7F) {
            /*One byte */
            if (count == 0) {
                bytes[count++] = read_value;
                encode = true;
                input_value = read_value;
				codepoint = read_value;
            }
            else {
                if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                    /*Unsafe action! Increment! */
                    safe_param = *(int*) ++saftey_ptr;
                    /* failed to move the file pointer back */
                    perror("NULL");
                }
                return false;
            }

       } else if (read_value > 0x80 && read_value < 0x7FF) {
            /*Two bytes*/
            if (count == 0) {
                int startbit = (6 << 5) | (read_value >> 6);
                int lastbit = (2 << 6) | (read_value & 0x3F);
                bytes[count++] = startbit;
                bytes[count++] = lastbit;
                encode = true;
                input_value = read_value;
				codepoint = read_value;
            } else {
                if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                    /*Unsafe action! Increment! */
                    safe_param = *(int*) ++saftey_ptr;
                    /* failed to move the file pointer back */
                    perror("NULL");
                }
                return false;
            }
       } else if (read_value > 0x800 && read_value < 0xFFFF) {

            
            /*Surrogate pair checker*/
            if (read_value >> 10 == 0x36) {
                 int doublebytes = read_value << 16;
				 input_value = read_value;
                if (read(input_fd, &surrogatechecker, 2) == 2) {
					if (sparky) 
						surrogatechecker = fliptoBEU(surrogatechecker);
                    if (inUTF16BE) {
                        unsigned int* surrogatept = &surrogatechecker;
						if (sparky)
							flipBitsBEU(surrogatept);
						else
							flipBitsU(surrogatept);
                    }
                    if (surrogatechecker >> 10 == 0x37) {
                        int surrogate = ((read_value & 0x3FF) << 10) | (surrogatechecker & 0x3FF);
                        surrogate += 0x10000;
                        read_value = surrogate;
						codepoint = surrogate;
                        doublebytes |= surrogate;
						input_value = doublebytes;
                         if (count == 0) {   
                            //code this part into bytes 
                            int fourthbyte = (2 << 6) | (surrogate & 0x3F);
                            int thirdbyte = (2 << 6) | ((surrogate >> 6) & 0x3F);
                            int secondbyte = (2 << 6) | ((surrogate >> 12) & 0x3F);
                            int firstbyte = (0x3E << 3) | ((surrogate >> 18));
                            bytes[count++] = firstbyte;
                            bytes[count++] = secondbyte;
                            bytes[count++] = thirdbyte;
                            bytes[count++] = fourthbyte;
                            encode = true;
                          
                        } else {
                            if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                                /*Unsafe action! Increment! */
                                safe_param = *(int*) ++saftey_ptr;
                                 /*failed to move the file pointer back */
                                perror("NULL");
                            }
                            return false;
                        }
                        goto finishchecker;
                    } else {
                        goto notsurrogatepair;
                    }
                } else {
                   goto notsurrogatepair;
                }
            } 
            

notsurrogatepair:
            if (count == 0) {
                int firstbit = (0xE << 4) | (read_value >> 12);
                int secondbit = (2 << 6) | (read_value >> 6);
                int thirdbit = (2 << 6) | (read_value & 0x3F);
                bytes[count++] = firstbit;
                bytes[count++] = secondbit;
                bytes[count++] = thirdbit;
				input_value = read_value;
				codepoint = read_value;
                encode = true;
            } else {
                if(lseek(input_fd, -1, SEEK_CUR) < 0) {
                    /*Unsafe action! Increment! */
                    safe_param = *(int*) ++saftey_ptr;
                    /* failed to move the file pointer back */
                    perror("NULL");
                }
                return false;
            }
       } else {
            fprintf(stderr, "%s\n", "Something went wrong");
            encode = false;    
       }

finishchecker:
			
            if (encode) {
                int i = 0;
				if (sparky) {
					 for(i = 0; i < count; i++) {
						 unsigned int value = bytes[i] << 24;
						
						 if(!safe_write(output_fd, &value, 1)) {
							fprintf(stderr, "%s\n", "Something went wrong with writing to file");
							exit(EXIT_FAILURE);
						}
						if (i == 0) {
							output_value = bytes[i];
						} else {
							output_value= (output_value << 8) | bytes[i]; 
						}
					}

					 

				} else {
					for(i = 0; i < count; i++) {
					int value = bytes[i];
					if(!safe_write(output_fd, &value, 1)) {
						fprintf(stderr, "%s\n", "Something went wrong with writing to file");
						exit(EXIT_FAILURE);
					}

					if (i == 0) {
						output_value = bytes[i];
					} else {
						output_value= (output_value << 8) | bytes[i]; 
					}
				}
				}
			 if (sparky) 
				 fliptoBEU(output_value);
        }
       
        if (inUTF16BE)
            verbose(v, read_value, count, codepoint, input_value, output_value);
        else if (inUTF16LE) {
            int temp = input_value;
            unsigned int* flip = &read_value;
			if (sparky)
				flipBitsBEU(flip);
			else
				flipBitsU(flip);
            verbose(v, temp, count, codepoint, *flip, output_value);
        } else {
            fprintf(stderr, "%s\n", "Invalid encoding!");
            exit(EXIT_FAILURE);
        }
        encode = false;
        count = 0;
        memset(bytes, 0, 4);
		read_value = 0;
		surrogatechecker = 0;
		input_value = 0;
		output_value = 0;
    }
    success = true;
    return success;
}

bool copyBytes(const int input_fd, const int output_fd, int buffer) {
    auto unsigned int read_value;
    verboseTop(v);
    /*Increment reader */
    read(input_fd, &read_value, buffer);
    while (read(input_fd, &read_value, 1) == 1) {
        if(!safe_write(output_fd, &read_value, 1)) {
                    fprintf(stderr, "%s\n", "Something went wrong with writing to file");
                    exit(EXIT_FAILURE);
        }

    }
    return true;
}


bool safe_write(int output_fd, void* value, size_t size) {
    bool success = true;
    ssize_t bytes_written;
    if((bytes_written = write(output_fd, value, size)) != size) {
        /* The write operation failed */
        fprintf(stdout, "Write to file failed. Expected %zu bytes but got %zd\n", size, bytes_written);
    }
    return success;
}
void verbose(int option, int ascii, int bytes, int codepoint, int input, int output) {
    
    if (ascii < 32 || ascii > 126) {
        char* asciiactual = "NONE";   
        if (option > 0) {
            switch(option) {
                case 1:
                    if (bytes != 4) {
                        fprintf(stderr, "|  %s    |      %d     |   U+%04X   |\n", asciiactual, bytes, codepoint);
                        fprintf(stderr, "%s\n", "+---------+------------+------------+");   
                    } else {
                        fprintf(stderr, "|  %s    |      %d     |   U+%05X   |\n", asciiactual, bytes, codepoint);
                        fprintf(stderr, "%s\n", "+---------+------------+-----------+");  
                    }
                    break;
                case 2:
                    if (bytes != 4) {
                        fprintf(stderr, "|  %s   |     %d      |   U+%04X   | 0x%04X|\n", asciiactual, bytes, codepoint, input);
                        fprintf(stderr, "%s\n", "+---------+------------+------------+-------+");
                    } else {
                        fprintf(stderr, "|  %s   |     %d      |   U+%05X   | 0x%04X|\n", asciiactual, bytes, codepoint, input);
                        fprintf(stderr, "%s\n", "+---------+------------+-----------+-------+");
                    }
                    break;
                
                default:
                    if (bytes != 4) {
                        fprintf(stderr, "|  %s   |     %d      |   U+%04X   | 0x%04X | 0x%04X |\n", asciiactual, bytes, codepoint, input, output);
                        fprintf(stderr, "%s\n", "+---------+------------+------------+--------+--------+");
                    } else {
                        fprintf(stderr, "|  %s   |     %d      |   U+%05X   | 0x%04X | 0x%04X |\n", asciiactual, bytes, codepoint, input, output);
                        fprintf(stderr, "%s\n", "+---------+------------+-----------+--------+-------+");
                    }
                    break;
            }
        }   
    } else {
        if (option > 0) {
            switch(option) {
                case 1:
                    if (bytes != 4) {
                        fprintf(stderr, "|   %c     |      %d     |   U+%04X   |\n", ascii, bytes, codepoint);
                        fprintf(stderr, "%s\n", "+---------+------------+------------+");   
                    } else {
                        fprintf(stderr, "|   %c     |      %d     |   U+%05X   |\n", ascii, bytes, codepoint);
                        fprintf(stderr, "%s\n", "+---------+------------+-----------+");  
                    }
                    break;
                case 2:
                    if (bytes != 4) {
                        fprintf(stderr, "|   %c     |     %d      |   U+%04X   | 0x%04X|\n", ascii, bytes, codepoint, input);
                        fprintf(stderr, "%s\n", "+---------+------------+------------+-------+");
                    } else {
                        fprintf(stderr, "|   %c     |     %d      |   U+%05X   | 0x%04X|\n", ascii, bytes, codepoint, input);
                        fprintf(stderr, "%s\n", "+---------+------------+-----------+-------+");
                    }
                    break;
                
                default:
                    if (bytes != 4) {
                        fprintf(stderr, "|   %c     |     %d      |   U+%04X   | 0x%04X | 0x%04X |\n", ascii, bytes, codepoint, input, output);
                        fprintf(stderr, "%s\n", "+---------+------------+------------+--------+--------+");
                    } else {
                        fprintf(stderr, "|   %c     |     %d      |   U+%05X   | 0x%04X | 0x%04X |\n", ascii, bytes, codepoint, input, output);
                        fprintf(stderr, "%s\n", "+---------+------------+-----------+--------+-------+");
                    }
                    break;
            }
        }   
    }   
}

void verboseTop(int option) {
    if (option > 0) {
        switch(option) {
            case 1:
                fprintf(stderr, "%s\n", 
                       "+---------+------------+------------+\n" \
                       "|  ASCII  | # of bytes | codepoint  |\n" \
                       "+---------+------------+------------+");
                break;
            case 2:
                fprintf(stderr, "%s\n", 
                       "+---------+------------+------------+-------+\n" \
                       "|  ASCII  | # of bytes | codepoint  | input |\n" \
                       "+---------+------------+------------+-------+");
                break;
            default:
                fprintf(stderr, "%s\n", 
                       "+---------+------------+------------+--------+--------+\n" \
                       "|  ASCII  | # of bytes | codepoint  | input  | output |\n" \
                       "+---------+------------+------------+--------+--------+");
                break;
        }
    }
}
void flipBits(int* n) {
    int value = *n;
    int firstbits = value & UTF16LE_MASKF;
    firstbits >>= 8;
    int lastbits = value & UTF16LE_MASKL;
    lastbits <<= 8;
    *n = firstbits | lastbits;
}

void flipBitsU(unsigned int* n) {
    int value = *n;
    int firstbits = value & UTF16LE_MASKF;
    firstbits >>= 8;
    int lastbits = value & UTF16LE_MASKL;
    lastbits <<= 8;
    *n = firstbits | lastbits;
}

void flipBitsBE(int* n) {
    int value = *n;
    int firstbits = value & 0xFF000000;
    firstbits >>=8;
    int secondbits = value & 0x00FF0000;
    secondbits <<=8;
    int firsttwobytes = firstbits | secondbits;
	int thirdbits = value & 0x0000FF00;
	thirdbits >>=8;
	int fourthbits = value & 0xFF;
	fourthbits <<=8;
	int lasttwobytes = thirdbits | fourthbits;
	*n = firsttwobytes | lasttwobytes;

}

void flipBitsBEU(unsigned int* n) {
    unsigned int value = *n;
    unsigned int firstbits = value & 0xFF000000;
    firstbits >>=8;
    unsigned int secondbits = value & 0x00FF0000;
    secondbits <<= 8;
    unsigned int firsttwobytes = firstbits | secondbits;
	unsigned int thirdbits = value & 0xFF00;
	thirdbits >>=8;
	unsigned int fourthbits = value & 0xFF;
	fourthbits <<=8;
	unsigned int lasttwobytes = thirdbits | fourthbits;
	*n = firsttwobytes | lasttwobytes;
}
void information() {
    char* inputencoding = NULL;
    char* outputencoding = NULL;

    if (inUTF16LE) 
        inputencoding = "UTF-16LE";
    else if (inUTF16BE)
        inputencoding = "UTF-16BE";
    else
        inputencoding = "UTF-8";

    if (outUTF8)
        outputencoding = "UTF-8";
    else if (outUTF16LE) 
        outputencoding = "UTF-16LE";
    else
        outputencoding = "UTF-16BE";

    struct stat st;
    stat(input_path, &st);
    int size = st.st_size;
    int inode = st.st_ino;
    int device = st.st_dev;
    info("Host ", hostname);
    input("Input File", input_path, inode, device, size);
    info("Output File", output_path);
    info("Input Encoding", inputencoding);
    info("Output Encoding", outputencoding); 
    size++;
    inode++;
    device++; 
    inputencoding++;
    outputencoding++;
}

int fliptoBE(int num) {
    int swapped = ((num>>24)&0xff) | // move byte 3 to byte 0
                  ((num<<8)&0xff0000) | // move byte 1 to byte 2
                  ((num>>8)&0xff00) | // move byte 2 to byte 1
                  ((num<<24)&0xff000000); // byte 0 to byte 3
    return swapped;
}

unsigned int fliptoBEU(int num) {
    unsigned int swapped = ((num>>24)&0xff) | // move byte 3 to byte 0
                  ((num<<8)&0xff0000) | // move byte 1 to byte 2
                  ((num>>8)&0xff00) | // move byte 2 to byte 1
                  ((num<<24)&0xff000000); // byte 0 to byte 3
    return swapped;
}