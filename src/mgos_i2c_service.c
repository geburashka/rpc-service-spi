/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>

#include "mgos_rpc.h"
#include "mgos_spi.h"
#include "mgos_hal.h"

#include "common/json_utils.h"
#include "common/mg_str.h"


static void spi_write_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                              struct mg_rpc_frame_info *fi,
                              struct mg_str args) {
  int bus = 0, addr = -1, len = -1;
  uint8_t *data = NULL;
  int err_code = 0;
  const char *err_msg = NULL;
  struct mgos_spi *spi;
  json_scanf(args.p, args.len, ri->args_fmt, &bus, &addr, &len, &data);
  if (addr < 0 || data == NULL) {
    err_code = 400;
    err_msg = "addr and data_hex are required";
    goto out;
  }
  spi = mgos_spi_get_bus(bus);
  if (spi == NULL) {
    err_code = 503;
    err_msg = "I2C is disabled";
    goto out;
  }
  if (!mgos_spi_write(spi, addr, data, len, true /* stop */)) {
    err_code = 503;
    err_msg = "I2C write failed";
  }
out:
  if (data != NULL) free(data);
  if (err_code != 0) {
    mg_rpc_send_errorf(ri, err_code, "%s", err_msg);
  } else {
    mg_rpc_send_responsef(ri, NULL);
  }
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}


bool mgos_rpc_service_spi_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, "I2C.Scan", "{bus: %d}", spi_scan_handler, NULL);
  mg_rpc_add_handler(c, "I2C.Read", "{bus: %d, addr: %d, len: %d}",
                     spi_read_handler, NULL);
  mg_rpc_add_handler(c, "I2C.Write", "{bus: %d, addr: %d, data_hex: %H}",
                     spi_write_handler, NULL);
  return true;
}
