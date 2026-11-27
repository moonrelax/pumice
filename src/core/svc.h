#ifndef PUMICE_SVC_H
#define PUMICE_SVC_H

#include "arm64.h"

#define SVC_SET_HEAP_SIZE           0x01
#define SVC_SET_MEMORY_ATTRIBUTE    0x03
#define SVC_MAP_MEMORY              0x06
#define SVC_QUERY_MEMORY            0x06
#define SVC_EXIT_PROCESS            0x07
#define SVC_CREATE_THREAD           0x08
#define SVC_START_THREAD            0x09
#define SVC_EXIT_THREAD             0x0A
#define SVC_SLEEP_THREAD            0x0B
#define SVC_GET_THREAD_PRIORITY     0x0C
#define SVC_SET_THREAD_PRIORITY     0x0D
#define SVC_CLOSE_HANDLE            0x16
#define SVC_RESET_SIGNAL            0x17
#define SVC_WAIT_SYNCHRONIZATION    0x18
#define SVC_CANCEL_SYNCHRONIZATION  0x19
#define SVC_ARBITRATE_LOCK          0x1A
#define SVC_ARBITRATE_UNLOCK        0x1B
#define SVC_WAIT_PROCESS_WIDE_KEY   0x1C
#define SVC_SIGNAL_PROCESS_WIDE_KEY 0x1D
#define SVC_CONNECT_TO_NAMED_PORT   0x1F
#define SVC_SEND_SYNC_REQUEST       0x21
#define SVC_GET_PROCESS_ID          0x24
#define SVC_GET_THREAD_ID           0x25
#define SVC_BREAK                   0x26
#define SVC_OUTPUT_DEBUG_STRING     0x27
#define SVC_GET_INFO                0x29
#define SVC_MAP_TRANSFER_MEMORY     0x51
#define SVC_QUERY_PHYSICAL_ADDRESS  0x53

#define RESULT_SUCCESS 0
#define RESULT_INVALID_HANDLE 0xE401

typedef struct kernel_state kernel_state;

void svc_handle(kernel_state *ks, arm64_cpu *cpu);

#endif
