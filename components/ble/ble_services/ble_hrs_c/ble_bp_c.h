
#ifndef BLE_BP_C_H__
#define BLE_BP_C_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"


#define BLE_UUID_BP_SERVICE 0x1810
#define BLE_BP_CONN_HANDLE 0x0

/*
#define BLE_BP_MEASUREMENT_HANDLE 0x4d
#define BLE_BP_MEASUREMENT_CCCD_HANDLE 0x4e
#define BLE_CUFF_HANDLE 0x52
#define BLE_CUFF_CCCD_HANDLE 0x53
*/

#define BLE_BP_MEASUREMENT_HANDLE 0xD 
#define BLE_BP_MEASUREMENT_CCCD_HANDLE 0xE
#define BLE_CUFF_HANDLE 0x10
#define BLE_CUFF_CCCD_HANDLE 0x11



#define bp_log printf
typedef enum
{
    BLE_BP_C_EVT_DISCOVERY_COMPLETE = 1,  /**< Event indicating that the Heart Rate Service has been discovered at the peer. */
    BLE_BP_C_EVT_MEA_NOTIFICATION,         /**< Event indicating that a notification of the Heart Rate Measurement characteristic has been received from the peer. */
    BLE_BP_C_EVT_CUFF_NOTIFICATION,
	  BLE_BP_C_EVT_GOT_VAL
} ble_bp_c_evt_type_t;

typedef struct ble_bp_c_s ble_bp_c_t;

typedef struct
{
    uint16_t bp_value[19];  /**< Heart Rate Value. */
} ble_bp_t;

typedef struct
{
    ble_bp_c_evt_type_t evt_type;  /**< Type of the event. */
    union
    {
        ble_bp_t bp;  /**< Heart rate measurement received. This will be filled if the evt_type is @ref BLE_HRS_C_EVT_HRM_NOTIFICATION. */
    } params;
} ble_bp_c_evt_t;

//typedef void (* ble_bp_c_evt_handler_t) (ble_bp_c_t * p_ble_bp_c, ble_bp_c_evt_t * p_evt);
typedef void (* ble_bp_c_evt_handler_t) (ble_bp_c_t * p_ble_bp_c, ble_bp_c_evt_t * p_evt);

struct ble_bp_c_s
{
    uint16_t                conn_handle;      /**< Connection handle as provided by the SoftDevice. */
    uint16_t                bp_CUFF_cccd_handle;  /**< Handle of the CCCD of the Heart Rate Measurement characteristic. */
    uint16_t                bp_MEA_cccd_handle;  /**< Handle of the CCCD of the Heart Rate Measurement characteristic. */
 // 	uint16_t                bp_handle;       /**< Handle of the Heart Rate Measurement characteristic as provided by the SoftDevice. */
	uint16_t                bp_CUFF_handle;
	uint16_t                bp_MEA_handle;
		uint16_t                bp_FEA_handle;
	  uint16_t                bp_test_handle;
	 uint16_t                bp_test_cccd_handle;
    ble_bp_c_evt_handler_t evt_handler;      /**< Application event handler to be called when there is an event related to the heart rate service. */
};


uint32_t ble_bp_c_init(ble_bp_c_t * p_ble_bp_c, ble_bp_c_evt_handler_t bp_callback);
uint32_t ble_bp_c_test_notif_enable(ble_bp_c_t * p_ble_bp_c);
void ble_bp_c_on_ble_evt(ble_bp_c_t * p_bp, ble_evt_t * p_ble_evt);
uint32_t ble_bp_c_mea_notif_enable(ble_bp_c_t * p_ble_bp_c);
uint32_t ble_bp_c_cuff_notif_enable(ble_bp_c_t * p_ble_bp_c);


#endif