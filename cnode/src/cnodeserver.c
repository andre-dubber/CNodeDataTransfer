/* cnode_s.c */

#include <string.h> /* memset */
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
void write_block_to_file(char *filename, char *data, int size);
lame_t setup();
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
//Pure C encoding that works fine
test_encode(lame_config);


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
  char part_ofilename[100];
  char part_encfilename[100];
  char part_ifilename[100];
  sprintf(part_ifilename, "/tmp/elixir_i%i.wav", part_no); 
  sprintf(part_ofilename, "/tmp/elixir_o%i.mp3", part_no); 
  int len, encoded_len; void *ptr;
  int write;
  char error[200];

  action = erl_element(1, tuplep);
  if (strncmp(ERL_ATOM_PTR(action), "mp3", 3) == 0) {
    filename = ERL_BIN_PTR(method_param);
    fprintf(stdout, "\n filename received %s", filename);
    fprintf(stdout, "\nLame encoding complete.\n\r");
    resp = erl_format("{cnode, ~s}", "/tmp/testcase.mp3");
  } else if (strncmp(ERL_ATOM_PTR(action), "encode", 6) == 0) {

    len = ERL_BIN_SIZE(method_param);
    ptr = ERL_BIN_PTR(method_param);
    if ( len > EXCHANGE_BUFSIZE ) {
      sprintf(error, "ERR: Length of the block %i exceeds maximum size %i", len, EXCHANGE_BUFSIZE);
      resp = erl_format("{error, ~s}", error);
      fprintf(stderr, "%s", error);
    } else {
      if( part_no < 5) {
        //fprintf(stdout, "\n Received block of %i bytes ", len);
        write_block_to_file(part_ifilename, (char *)ptr, len);
      }

      encoded_len = lame_encode_buffer(lame_config, (short int *) ptr, (short int *) ptr, len, output_buffer, EXCHANGE_BUFSIZE);
      if( part_no < 5) 
        write_block_to_file(part_ofilename, (char *)output_buffer, encoded_len);
      resp = erl_format("{cnode, ~w}", erl_mk_binary(output_buffer, encoded_len));
    }
  }
  erl_free_term(action);

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
  unsigned char input_buffer[EXCHANGE_BUFSIZE];
  unsigned char output_buffer[EXCHANGE_BUFSIZE];

  do {
    sprintf(part_ifilename, "/tmp/tef_i%i.wav", part_no); 
    sprintf(part_ofilename, "/tmp/tef_o%i.mp3", part_no); 

    read = fread(input_buffer, 1, EXCHANGE_BUFSIZE, pcm);
    
    if( part_no < 5) 
      write_block_to_file(part_ifilename, (char *)input_buffer, read);

    encoded_len = lame_encode_buffer(lame_config, (short int *)input_buffer, (short int *)input_buffer, read, output_buffer, EXCHANGE_BUFSIZE);
    write = fwrite(output_buffer, encoded_len, 1, mp3);

    if( part_no < 5) 
      write_block_to_file(part_ofilename, output_buffer, encoded_len);
    part_no++;
  } while (read != 0);

  fclose(mp3);
  fclose(pcm);
}

void write_block_to_file(char *filename, char* data, int size) {
  FILE *file = fopen(filename, "wb");
  if (file == NULL)
  {
      perror( filename );
      exit(0);
  }
  fwrite(data, size, 1, file);
  fclose(file);
}

void print_error(const char *format, va_list ap) {
  fprintf(stderr, format, ap);
}

lame_t setup() {
  lame_t lame = lame_init();
  lame_set_VBR(lame, vbr_default);
  lame_set_in_samplerate(lame, 8000);
  lame_set_num_channels(lame, 1);
  lame_set_out_samplerate(lame, 8000);
  lame_set_mode(lame, 3);
  lame_init_params(lame);
  lame_set_errorf(lame, print_error);
  return lame;
}

