/* cnode_s.c */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include "erl_interface.h"
#include "ei.h"
#include "../include/encoder.h"

#define BUFSIZE 1000
#define EXCHANGE_BUFSIZE 8192

void test_encode(lame_t lame_config);
ETERM *process_block_encoding(ETERM *tuplep, ETERM *method_param, lame_t lame_config, int offset);

int main(int argc, char **argv) {
  fprintf(stdout, "\nStarting up server");
  fprintf(stdout, "\nDevision result: %i", 0%EXCHANGE_BUFSIZE);

  int port;                                /* Listen port number */
  int listen;                              /* Listen socket */
  int fd;                                  /* fd to Erlang node */
  ErlConnect conn;                         /* Connection data */

  int loop = 1;                            /* Loop flag */
  int got;                                 /* Result of receive */
  unsigned char buf[BUFSIZE];              /* Buffer for incoming message */
  ErlMessage emsg;                         /* Incoming message */
  lame_t lame_config;

  ETERM *fromp, *tuplep, *argp, *resp, *method_param;
  int res, offset;

  // Initialize lame config
  fprintf(stdout, "\nCalling lame setup...\n\r");
  lame_config = setup();
  fprintf(stdout, "\nLame setup cmplete.\n\r");
test_encode(lame_config);
//encode_file2(lame_config, "/tmp/1.wav", "/tmp/enc_file2.mp3");


  port = atoi(argv[1]);

  fprintf(stdout, "\nInitializing Erlang");
  erl_init(NULL, 0);

  if (erl_connect_init(1, "secretcookie", 0) == -1)
    erl_err_quit("erl_connect_init");

  /* Make a listen socket */
  if ((listen = my_listen(port)) <= 0)
    erl_err_quit("my_listen");

  if (erl_publish(port) == -1)
    erl_err_quit("erl_publish");

  if ((fd = erl_accept(listen, &conn)) == ERL_ERROR)
    erl_err_quit("erl_accept");
  fprintf(stdout, "Connected to %s\n\r", conn.nodename);
  offset = 0;
  while (loop) {

    got = erl_receive_msg(fd, buf, BUFSIZE, &emsg);
    if (got == ERL_TICK) {
      /* ignore */
      fprintf(stdout, ".");
    } else if (got == ERL_ERROR) {
      fprintf(stderr, "\nGot ERL_ERROR.");
      loop = 0;
    } else {

      //fprintf(stdout, "\nGot payload.");
      if (emsg.type == ERL_REG_SEND) {
        fromp = erl_element(2, emsg.msg);
        tuplep = erl_element(3, emsg.msg);
        method_param = erl_element(2, tuplep);
        if ( !ERL_IS_BINARY(method_param)) {
          resp = erl_format("{error, ~s}", "Received non-binary parameter");
          fprintf(stderr, "filename is not a binary");
        }
        else {
          test_encode2(lame_config, offset);
          resp = process_block_encoding(tuplep, method_param, lame_config, offset);
          offset += EXCHANGE_BUFSIZE;
        }

        //fprintf(stdout, "\nPrepared result value.\n\r");
        erl_send(fd, fromp, resp);
        //fprintf(stdout, "\nResult value sent.\n\r");

        erl_free_term(emsg.from);
        erl_free_term(emsg.msg);
        erl_free_term(fromp);
        erl_free_term(tuplep);
        erl_free_term(method_param);
        erl_free_term(resp);
        //fprintf(stdout, "\nRequest completed.\n\r");
      }
    }
  } /* while */
}


int my_listen(int port) {
  int listen_fd;
  struct sockaddr_in addr;
  int on = 1;

  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return (-1);

  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  //memset((void*) &addr, 0, (size_t) sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(listen_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    return (-1);

  listen(listen_fd, 5);
  return listen_fd;
}

void print_mem(void const *vp, size_t n)
{
    unsigned char const *p = vp;
    size_t i;
    for (i=0; i<n; i++)
        printf("%02x\n", p[i]);
    putchar('\n');
}

ETERM *process_block_encoding(ETERM *tuplep, ETERM *method_param, lame_t lame_config, int offset) {
  ETERM *action, *resp;
  char *filename;
  short int input_buffer[EXCHANGE_BUFSIZE/2];
  uint8_t bytes_buffer[EXCHANGE_BUFSIZE];
  unsigned char output_buffer[EXCHANGE_BUFSIZE/2];
  int part_no = offset/EXCHANGE_BUFSIZE;
if (part_no > 5) part_no = 5;
  char part_ofilename[100];
  char part_encfilename[100];
  char part_ifilename[100];
  sprintf(part_ifilename, "/tmp/elixir_i%i.wav", part_no); 
  sprintf(part_ofilename, "/tmp/elixir_o%i.mp3", part_no); 
  sprintf(part_encfilename, "/tmp/elixir_enc%i.mp3", part_no); 
  int len, encoded_len; void *ptr;
  int write;
  FILE *mp3_ipartial = fopen(part_ifilename, "wb");
  FILE *mp3_opartial = fopen(part_ofilename, "wb");
  FILE *mp3_encpartial = fopen(part_encfilename, "wb");
  FILE *pcm = fopen("/tmp/l.wav", "ab");
  FILE *mp3 = fopen("/tmp/l.mp3", "ab");

  action = erl_element(1, tuplep);
  if (strncmp(ERL_ATOM_PTR(action), "mp3", 3) == 0) {
    filename = ERL_BIN_PTR(method_param);
    fprintf(stdout, "\n filename received %s", filename);
    encode_file(lame_config, filename, "/tmp/testcase.mp3");
    fprintf(stdout, "\nLame encoding complete.\n\r");
    resp = erl_format("{cnode, ~s}", "/tmp/testcase.mp3");
  } else if (strncmp(ERL_ATOM_PTR(action), "encode", 6) == 0) {

    len = ERL_BIN_SIZE(method_param);
    ptr = ERL_BIN_PTR(method_param);
    if ( len > EXCHANGE_BUFSIZE ) {
      resp = erl_format("{error, ~s}", sprintf("ERR: Length of the block %i exceeds maximum size %i", len, EXCHANGE_BUFSIZE));
      fprintf(stderr, "\n ERR: Length of the block %i exceeds maximum size %i", len, EXCHANGE_BUFSIZE);
      return resp;
    } else {
      memset(input_buffer, '\0', sizeof(input_buffer));
      memset(bytes_buffer, '\0', sizeof(bytes_buffer));
      memcpy(bytes_buffer, ptr, len);
      memcpy(input_buffer, ptr, len);
      write = fwrite(bytes_buffer, len, 1, mp3_ipartial);
//fprintf(stdout, "Data sizes, received %i short int: %i", sizeof(bytes_buffer[0]), sizeof(short int));
      //int i;
      //for (i = 0; i < len; i += 2)
        //input_buffer[i/2] = bytes_buffer[i+1] | (short int)bytes_buffer[i] << 8; //big endian
        //input_buffer[i/2] = (bytes_buffer[i] << 8) | bytes_buffer[i+1];
        //input_buffer[i/2] = bytes_buffer[i] | (short int)bytes_buffer[i+1] << 8; //little endian
        //input_buffer[i] = (int)bytes_buffer[i];
      //write = fwrite(bytes_buffer, len, 1, pcm);
      if( part_no < 3)
      if (  memcmp(input_buffer, bytes_buffer, len)) {
        fprintf(stderr, "\n Arrays match after casting ");
      } else {
        fprintf(stderr, "\n Arrays DONT match after casting ");
      }
      write = fwrite(input_buffer, len, 1, mp3_encpartial);
    }
    //fprintf(stdout, "\nBlock received %i bytes", len);
    encoded_len = lame_encode_buffer(lame_config, input_buffer, input_buffer, len, output_buffer, len);

    //fprintf(stdout, "\n Encoded block, result size %i bytes", encoded_len);
    //Log to l.mp3 file
    write = fwrite(output_buffer, encoded_len, 1, mp3);
    write = fwrite(output_buffer, encoded_len, 1, mp3_opartial);
    resp = erl_format("{cnode, ~w}", erl_mk_binary(output_buffer, encoded_len));
  }
  erl_free_term(action);

  fclose(mp3);
  fclose(mp3_opartial);
  fclose(mp3_encpartial);
  fclose(mp3_ipartial);
  fclose(pcm);
  return resp;
}

void test_encode(lame_t lame_config) {
  fprintf(stdout, "\nStarting test encode");
  fprintf(stdout, "\nSetup complete");
  int read, write, encoded_len;
  FILE *pcm = fopen("/tmp/1.wav", "rb");
  FILE *mp3 = fopen("/tmp/full_file_encode.mp3", "wb");

  int part_no = 0;
  char part_ofilename[100];
  char part_ifilename[100];
  short int input_buffer[EXCHANGE_BUFSIZE];
  unsigned char output_buffer[EXCHANGE_BUFSIZE];

  do {
    sprintf(part_ifilename, "/tmp/tef_i%i.wav", part_no); 
    sprintf(part_ofilename, "/tmp/tef_o%i.mp3", part_no); 

    read = fread(input_buffer, sizeof(short int), EXCHANGE_BUFSIZE, pcm);
    //fprintf(stdout, "Read  %i %i", read, sizeof(input_buffer));
    
    if( part_no < 5) 
      write_block_to_file(part_ifilename, input_buffer, read);

    encoded_len = lame_encode_buffer(lame_config, input_buffer, input_buffer, EXCHANGE_BUFSIZE, output_buffer, EXCHANGE_BUFSIZE);
    write = fwrite(output_buffer, encoded_len, 1, mp3);

    if( part_no < 5) 
      write_block_to_file(part_ofilename, output_buffer, encoded_len);
    part_no++;
  } while (read != 0);

  fclose(mp3);
  fclose(pcm);
}

void write_block_to_file(char *filename, short int data[], int size) {
fprintf(stdout, "\nWriting block to file '%s' ", filename);
  FILE *file = fopen(filename, "wb");
  if (file == NULL)
  {
      //printf("Oh dear, something went wrong with fopen()! %s\n", strerror_r(errno));
      printf("Oh dear, something went wrong with fopen()! \n" );
      perror( filename );
      exit(0);
  }
  fwrite(data, size, 1, file);
  fclose(file);
}

void test_encode2(lame_t lame_config, int offset) {
  //fprintf(stdout, "\nStarting test encode2 with offset %i", offset);
  //fprintf(stdout, "\nSetup complete");
  int read, write, total_read, len;
  int part_no = offset/EXCHANGE_BUFSIZE;
if (part_no > 5) part_no = 5;
  char part_ofilename[100];
  char part_ifilename[100];
  sprintf(part_ifilename, "/tmp/te_i%i.wav", part_no); 
  sprintf(part_ofilename, "/tmp/te_o%i.mp3", part_no); 
  //fprintf(stdout, "\nWriting part no %i ", part_no);
  //fprintf(stdout, "\nWriting part no %i file: %s", part_no, part_filename);
  
  FILE *pcm = fopen("/tmp/1.wav", "rb");
  FILE *mp3_ipartial = fopen(part_ifilename, "wb");
  FILE *mp3_opartial = fopen(part_ofilename, "wb");
  FILE *mp3 = fopen("/tmp/te.mp3", "ab");

  short int input_buffer[EXCHANGE_BUFSIZE/2];
  unsigned char output_buffer[EXCHANGE_BUFSIZE/2];
  total_read = 0;

  do {
    read = fread(input_buffer, sizeof(short int), EXCHANGE_BUFSIZE/2, pcm);

    //fprintf(stdout, "\nReading 1.wav in test. Read %i total read %i offset %i", read, total_read, offset);
    if ( total_read == offset ) {
      //fprintf(stdout, "\nDeteceted need to encode at Read %i total read %i offset %i", read, total_read, offset);
      write = fwrite(input_buffer, read, 1, mp3_ipartial);
      len = lame_encode_buffer(lame_config, input_buffer, input_buffer, EXCHANGE_BUFSIZE/2, output_buffer, EXCHANGE_BUFSIZE/2);
      write = fwrite(output_buffer, len, 1, mp3);
      write = fwrite(output_buffer, len, 1, mp3_opartial);
      fclose(mp3);
      fclose(mp3_ipartial);
      fclose(mp3_opartial);
      fclose(pcm);
      return;
    } else {
      total_read += read;
    }

  } while (read != 0);

  fclose(mp3);
  fclose(mp3_opartial);
  fclose(mp3_ipartial);
  fclose(pcm);
}



