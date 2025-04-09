//#define DEBUG_SOCKET
#define DEBUG_IP "192.168.2.2"
#define DEBUG_PORT 9023
#include "pad.h"

#define PS3_STRATOCASTER_VENDOR_ID 0x12BA
#define PS3_STRATOCASTER_PRODUCT_ID 0x0200

#define PS_BUTTON 0x10000
#define PS4_STRATOCASTER_VENDOR_ID 0x0738
#define PS4_STRATOCASTER_PRODUCT_ID 0x8261
#define STR_MANUFACTURER "Mad Catz, Inc."
#define STR_PRODUCT "Mad Catz Guitar for RB4"

#include "ps4.h"

int _main(struct thread *td) {
  UNUSED(td);

  initKernel();
  initLibc();

#ifdef DEBUG_SOCKET
  initNetwork();
  DEBUG_SOCK = SckConnect(DEBUG_IP, DEBUG_PORT);
#endif

  jailbreak();

  initSysUtil();

  printf_notification("Running in kernel mode");

#ifdef DEBUG_SOCKET
  printf_debug("Closing socket...\n");
  SckClose(DEBUG_SOCK);
#endif

  return 0;
}
