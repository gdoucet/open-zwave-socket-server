#include <stdio.h>
#include <libconfig.h>

int main(int argc, char* argv[])
{
  config_t cfg;
  config_setting_t *setting;
  const char *cmd = "";
  long int port;

  char *config_file_name = "server.cfg";

  /* Initialization */
  config_init(&cfg);

  /* Read the file. If there is an error, report it and exit. */
  if (!config_read_file(&cfg, config_file_name))
  {
    printf("Error unable to process config file\n\n");
    config_destroy(&cfg);
    return -1;
  }

  if (config_lookup_int(&cfg, "port", &port))
  {
    printf("\nPort to run server on: %lu", port);
  } else {
    port = 6004;
  }

  if (config_lookup_string(&cfg, "cmd", &cmd))
  {
    printf("\nCommand to execute for events: %s", cmd);
  } else {
    cmd = "";
  }


  return 0;
}
