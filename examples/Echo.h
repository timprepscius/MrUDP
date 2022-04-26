#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int run_echo_server_c(int argc, const char *argv[]);
int run_echo_server_cpp(int argc, const char *argv[]);

int run_echo_client_c(int argc, const char *argv[]);
int run_echo_client_cpp(int argc, const char *argv[]);

#ifdef __cplusplus
}
#endif
