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


static void spi_run_txn_handler(struct mg_rpc_request_info *ri, void *cb_arg,
                                struct mg_rpc_frame_info *fi,
                                struct mg_str args) {
  int bus = 0, addr = -1, len = -1;
  uint8_t *data = NULL;
  int err_code = 0;
  const char *err_msg = NULL;
  struct mgos_spi *spi;

  json_scanf(args.p, args.len, ri->args_fmt, &bus, &addr, &len, &data);
/*
  if (addr < 0 || data == NULL) {
    err_code = 400;
    err_msg = "addr and data_hex are required";
    goto out;
  }
*/

  spi = mgos_spi_get_global();
  if (spi == NULL) {
    err_code = 503;
    err_msg = "SPI is disabled";
    goto out;
  }

  struct mgos_spi_txn txn = {
    .cs = 0,
    .mode = 0,
    .freq = 10000000,
  };

  uint8_t tx_data[1] = {0x9f};
  uint8_t rx_data[3] = {0, 0, 0};

  /* Half-duplex, command/response transaction setup */
  bool fd = false;
  /* Transmit 1 byte from tx_data. */
  txn.hd.tx_len = 0;
  txn.hd.tx_data = tx_data;
  /* No dummy bytes necessary. */
  txn.hd.dummy_len = 0;
  /* Receive 3 bytes into rx_data. */
  txn.hd.rx_len = 3;
  txn.hd.rx_data = rx_data;

  if (!mgos_spi_run_txn(spi, fd, &txn)) {
    err_code = 503;
    err_msg = "SPI write failed";
  }

out:
  if (data != NULL) free(data);
  if (err_code != 0) {
    mg_rpc_send_errorf(ri, err_code, "%s", err_msg);
  } else {
    mg_rpc_send_responsef(ri, "{rx_data: %H}", txn.hd.rx_len, txn.hd.rx_data);
  }
  ri = NULL;
  (void) cb_arg;
  (void) fi;
}


bool mgos_rpc_service_spi_init(void) {
  struct mg_rpc *c = mgos_rpc_get_global();
  mg_rpc_add_handler(c, "SPI.Run", "{len: %d}",
                     spi_run_txn_handler, NULL);
  return true;
}
