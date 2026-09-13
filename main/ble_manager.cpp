#include "ble_manager.h"
#include "config.h"

#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <cstring>

namespace {
constexpr char TAG[] = "ble";
bool paired = false;
uint16_t connection_handle = BLE_HS_CONN_HANDLE_NONE;

// Service: ESP32 Music Player 2.0
static const ble_uuid128_t service_uuid = BLE_UUID128_INIT(
    0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,
    0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00,0x01);
static const ble_uuid128_t code_uuid = BLE_UUID128_INIT(
    0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,
    0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00,0x02);
static const ble_uuid128_t status_uuid = BLE_UUID128_INIT(
    0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,
    0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00,0x03);
static const ble_uuid128_t command_uuid = BLE_UUID128_INIT(
    0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,
    0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0x00,0x04);

static int access_cb(uint16_t, uint16_t, struct ble_gatt_access_ctxt* ctxt, void*) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        const ble_uuid_t* uuid = ctxt->chr->uuid;
        if (ble_uuid_cmp(uuid, &code_uuid.u) == 0) {
            os_mbuf_append(ctxt->om, PLAYER_CONNECT_CODE, strlen(PLAYER_CONNECT_CODE));
            return 0;
        }
        if (ble_uuid_cmp(uuid, &status_uuid.u) == 0) {
            const char* status = paired ? "paired" : "locked";
            os_mbuf_append(ctxt->om, status, strlen(status));
            return 0;
        }
        return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR && ble_uuid_cmp(ctxt->chr->uuid, &command_uuid.u) == 0) {
        char value[32] = {};
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len >= sizeof(value)) len = sizeof(value) - 1;
        ble_hs_mbuf_to_flat(ctxt->om, value, len, nullptr);
        value[len] = '\0';

        // First command after connecting must be AUTH:<6-char-code>.
        if (strncmp(value, "AUTH:", 5) == 0 && strcmp(value + 5, PLAYER_CONNECT_CODE) == 0) {
            paired = true;
            ESP_LOGI(TAG, "Application authenticated");
            return 0;
        }
        if (!paired) return BLE_ATT_ERR_INSUFFICIENT_AUTHEN;
        ESP_LOGI(TAG, "Command received: %s", value);
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &service_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            { .uuid = &code_uuid.u, .access_cb = access_cb, .flags = BLE_GATT_CHR_F_READ },
            { .uuid = &status_uuid.u, .access_cb = access_cb, .flags = BLE_GATT_CHR_F_READ },
            { .uuid = &command_uuid.u, .access_cb = access_cb, .flags = BLE_GATT_CHR_F_WRITE },
            { 0 }
        },
    },
    { 0 }
};

static void advertise() {
    ble_hs_adv_fields fields{};
    const char* name = "ESP32-Music-Player";
    fields.name = reinterpret_cast<const uint8_t*>(name);
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    ble_gap_adv_set_fields(&fields);

    ble_gap_adv_params params{};
    params.conn_mode = BLE_GAP_CONN_MODE_UND;
    params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, nullptr, BLE_HS_FOREVER, &params, nullptr, nullptr);
}

static int gap_event(struct ble_gap_event* event, void*) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            connection_handle = event->connect.status == 0 ? event->connect.conn_handle : BLE_HS_CONN_HANDLE_NONE;
            paired = false;
            if (event->connect.status != 0) advertise();
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            paired = false;
            connection_handle = BLE_HS_CONN_HANDLE_NONE;
            advertise();
            break;
        case BLE_GAP_EVENT_ADV_COMPLETE:
            advertise();
            break;
        default:
            break;
    }
    return 0;
}

static void on_sync() {
    uint8_t addr_type;
    ble_hs_id_infer_auto(0, &addr_type);
    advertise();
}

static void host_task(void*) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}
}

bool ble_init() {
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set("ESP32-Music-Player");
    ble_gatts_count_cfg(services);
    ble_gatts_add_svcs(services);
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    nimble_port_freertos_init(host_task);
    ESP_LOGI(TAG, "BLE initialized; connect code is %s", PLAYER_CONNECT_CODE);
    return true;
}

bool ble_is_paired() { return paired; }
