#include "BleReceiver.hpp"
#include "esp_check.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "os/os_mbuf.h"
#include <cassert>
namespace vslink {
namespace {
BleReceiver::Write write_handler;
BleReceiver::Connection connection_handler;
uint8_t address_type;
const ble_uuid128_t service = BLE_UUID128_INIT(0x31,0xb6,0x6d,0xfc,0x43,0x3e,0x9d,0xb4,0x2f,0x4f,0x66,0xd6,0xea,0xb3,0x49,0xd7);
const ble_uuid128_t input = BLE_UUID128_INIT(0x32,0xb6,0x6d,0xfc,0x43,0x3e,0x9d,0xb4,0x2f,0x4f,0x66,0xd6,0xea,0xb3,0x49,0xd7);
int access(uint16_t,uint16_t,ble_gatt_access_ctxt *ctx,void *) {
  if (ctx->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
  const unsigned length=OS_MBUF_PKTLEN(ctx->om);
  if (!length || length>512) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  char command[512];
  if (os_mbuf_copydata(ctx->om,0,length,command)) return BLE_ATT_ERR_UNLIKELY;
  return write_handler(command,length) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}
void advertise();
int gap(ble_gap_event *event,void *) {
  switch(event->type) {
    case BLE_GAP_EVENT_CONNECT:
      if (event->connect.status) advertise();
      else {
        connection_handler(true);
        ble_gap_upd_params p{};
        p.itvl_min=6; p.itvl_max=12; p.latency=0; p.supervision_timeout=200;
        ble_gap_update_params(event->connect.conn_handle,&p);
      }
      break;
    case BLE_GAP_EVENT_DISCONNECT: connection_handler(false); advertise(); break;
    case BLE_GAP_EVENT_ADV_COMPLETE: advertise(); break;
    default: break;
  }
  return 0;
}
void advertise() {
  ble_hs_adv_fields fields{};
  fields.flags=BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  fields.uuids128=const_cast<ble_uuid128_t *>(&service); fields.num_uuids128=1; fields.uuids128_is_complete=1;
  int rc=ble_gap_adv_set_fields(&fields);
  if (rc) { ESP_LOGE("BLE","Advertising data error %d",rc); return; }
  // Put the name in the scan response; a 128-bit UUID leaves little legacy ADV space.
  ble_hs_adv_fields scan{};
  const char name[]="VSLink";
  scan.name=reinterpret_cast<const uint8_t *>(name); scan.name_len=sizeof(name)-1; scan.name_is_complete=1;
  ble_gap_adv_rsp_set_fields(&scan);
  ble_gap_adv_params params{};
  params.conn_mode=BLE_GAP_CONN_MODE_UND; params.disc_mode=BLE_GAP_DISC_MODE_GEN;
  params.itvl_min=32; params.itvl_max=48;
  rc=ble_gap_adv_start(address_type,nullptr,BLE_HS_FOREVER,&params,gap,nullptr);
  if (rc) ESP_LOGE("BLE","Advertising start error %d",rc);
}
void sync() {
  int rc=ble_hs_util_ensure_addr(0); assert(rc==0);
  rc=ble_hs_id_infer_auto(0,&address_type); assert(rc==0);
  advertise();
}
void host(void *) { nimble_port_run(); nimble_port_freertos_deinit(); }
void host_reset(int) { connection_handler(false); }
}
void BleReceiver::start(Write write,Connection connection) {
  write_handler=write; connection_handler=connection;
  ESP_ERROR_CHECK(nimble_port_init());
  ble_svc_gap_init(); ble_svc_gatt_init();
  ble_svc_gap_device_name_set("VSLink");
  static ble_gatt_chr_def characteristics[2]{};
  characteristics[0].uuid=&input.u; characteristics[0].access_cb=access;
  characteristics[0].flags=BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP;
  static ble_gatt_svc_def services[2]{};
  services[0].type=BLE_GATT_SVC_TYPE_PRIMARY; services[0].uuid=&service.u; services[0].characteristics=characteristics;
  int rc=ble_gatts_count_cfg(services); assert(rc==0);
  rc=ble_gatts_add_svcs(services); assert(rc==0);
  ble_hs_cfg.sync_cb=sync; ble_hs_cfg.reset_cb=host_reset;
  nimble_port_freertos_init(host);
}
}
