#include "ble_bp_c.h"

#include <stdint.h>
#include "ble_hrs_c.h"
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "nordic_common.h"
#include "nrf_error.h"
#include "ble_gattc.h"
#include "app_util.h"
#include "app_trace.h"

static ble_bp_c_t * mp_ble_bp_c;


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
#define BLE_CCCD_VALUE_LEN                                       2          /**< The length of a CCCD value. */
#define WRITE_MESSAGE_LENGTH   BLE_CCCD_VALUE_LEN 
typedef enum
{
    READ_REQ,  /**< Type identifying that this tx_message is a read request. */
    WRITE_REQ  /**< Type identifying that this tx_message is a write request. */
} tx_request_t;

/**@brief Structure for writing a message to the peer, i.e. CCCD.
 */
typedef struct
{
    uint8_t                  gattc_value[WRITE_MESSAGE_LENGTH];  /**< The message to write. */
    ble_gattc_write_params_t gattc_params;                       /**< GATTC parameters for this message. */
} write_params_t;
typedef struct
{
    uint16_t     conn_handle;  /**< Connection handle to be used when transmitting this message. */
    tx_request_t type;         /**< Type of this message, i.e. read or write message. */
    union
    {
        uint16_t       read_handle;  /**< Read request message. */
        write_params_t write_req;    /**< Write request message. */
    } req;
} tx_message_t;
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#define TX_BUFFER_MASK         0x07                  /**< TX Buffer mask, must be a mask of continuous zeroes, followed by continuous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE         (TX_BUFFER_MASK + 1)
static tx_message_t  m_tx_buffer[TX_BUFFER_SIZE];  /**< Transmit buffer for messages to be transmitted to the central. */
static uint32_t      m_tx_insert_index = 0; 
static uint32_t      m_tx_index = 0;

static void tx_buffer_process(void)
{
	//printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\r\n");
	//printf("m_tx_index=%d,m_tx_insert_index=%d\r\n",m_tx_index,m_tx_insert_index);
	//printf("m_tx_buffer[m_tx_index].type=%d\r\n",m_tx_buffer[m_tx_index].type);
    if (m_tx_index != m_tx_insert_index)
    {
        uint32_t err_code;

        if (m_tx_buffer[m_tx_index].type == READ_REQ)
        {
            err_code = sd_ble_gattc_read(m_tx_buffer[m_tx_index].conn_handle,
                                         m_tx_buffer[m_tx_index].req.read_handle,
                                         0);
        }
        else
        {
            err_code = sd_ble_gattc_write(m_tx_buffer[m_tx_index].conn_handle,
                                          &m_tx_buffer[m_tx_index].req.write_req.gattc_params);
        }
        if (err_code == NRF_SUCCESS)
        {
            //printf("[BP_C]: SD Read/Write API returns Success..\r\n");
            m_tx_index++;
            m_tx_index &= TX_BUFFER_MASK;
        }
        else
        {
            //printf("[BP_C]: SD Read/Write API returns error. This message sending will be "
            //    "attempted again..\r\n");
        }
    }
}

extern void bp_c_evt_handler(ble_bp_c_t * p_bp_c, ble_bp_c_evt_t * p_bp_c_evt);
static void on_hvx(ble_bp_c_t * p_ble_bp_c, const ble_evt_t * p_ble_evt)
{
	bp_log("p_ble_evt->evt.gattc_evt.params.hvx.handle=0x%x\r\n",p_ble_evt->evt.gattc_evt.params.hvx.handle);
	  bp_log("p_ble_hrs_c->bp_CUFF_handle=0x%x\r\n",p_ble_bp_c->bp_CUFF_handle);
	  bp_log("p_ble_hrs_c->bp_MEA_handle=0x%x\r\n",p_ble_bp_c->bp_MEA_handle);
	
    p_ble_bp_c->bp_CUFF_handle = BLE_CUFF_HANDLE;
	 p_ble_bp_c->bp_MEA_handle = BLE_BP_MEASUREMENT_HANDLE;
    // Check if this is a heart rate notification.
    //if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_bp_c->bp_CUFF_handle)
			 if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_bp_c->bp_MEA_handle)
    {
        ble_bp_c_evt_t ble_bp_c_evt;
        uint32_t        index = 0;

        ble_bp_c_evt.evt_type = BLE_BP_C_EVT_GOT_VAL;
			
			bp_log("hvx:len=0x%x\r\n",(p_ble_evt->evt.gattc_evt.params.hvx.len));
			int ii;
			//for (ii=0;ii<p_ble_evt->evt.gattc_evt.params.hvx.len;ii++){
				for (ii=0;ii<4;ii++){
					ble_bp_c_evt.params.bp.bp_value[ii] = p_ble_evt->evt.gattc_evt.params.hvx.data[index+ii];
			    bp_log("hvx:index=%d, val=%d\r\n",index+ii,p_ble_evt->evt.gattc_evt.params.hvx.data[index+ii]);
			}
			 //bp_log("hvx:index=%d, val=0x%x\r\n",index+len,p_ble_evt->evt.gattc_evt.params.hvx.data[index+len]);
			bp_log("Finish\r\n");
			
			bp_c_evt_handler(p_ble_bp_c, &ble_bp_c_evt);
			
			
/*
        if (!(p_ble_evt->evt.gattc_evt.params.hvx.data[index++] & HRM_FLAG_MASK_HR_16BIT))
        {
            // 8 Bit heart rate value received.
            ble_bp_c_evt.params.hrm.hr_value = p_ble_evt->evt.gattc_evt.params.hvx.data[index++];  //lint !e415 suppress Lint Warning 415: Likely access out of bond
        }
        else
        {
            // 16 bit heart rate value received.
            ble_bp_c_evt.params.hrm.hr_value =
                uint16_decode(&(p_ble_evt->evt.gattc_evt.params.hvx.data[index]));
        }

        p_ble_hrs_c->evt_handler(p_ble_hrs_c, &ble_hrs_c_evt);
*/ 
   }
}


static uint32_t NOTIFICATION_cccd_configure(uint16_t conn_handle, uint16_t handle_cccd, bool enable)
{
   //printf("[BP]: Configuring CCCD. CCCD Handle = %d, Connection Handle = %d\r\n", handle_cccd,conn_handle);
	
	  tx_message_t * p_msg;
    uint16_t       cccd_val = enable ? BLE_GATT_HVX_NOTIFICATION : 0;
    //printf("cccd_val=0x%x\\r\n",cccd_val);
    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = handle_cccd;
    p_msg->req.write_req.gattc_params.len      = WRITE_MESSAGE_LENGTH;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    p_msg->req.write_req.gattc_value[0]        = LSB(cccd_val);
    p_msg->req.write_req.gattc_value[1]        = MSB(cccd_val);
    p_msg->conn_handle                         = conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    
	  return NRF_SUCCESS;
}

static uint32_t INDICATION_cccd_configure(uint16_t conn_handle, uint16_t handle_cccd, bool enable)
{
   //printf("[BP]: Configuring CCCD. CCCD Handle = %d, Connection Handle = %d\r\n", handle_cccd,conn_handle);
	
	  tx_message_t * p_msg;
    uint16_t       cccd_val = enable ? BLE_GATT_HVX_INDICATION : 0;
    //printf("cccd_val=0x%x\\r\n",cccd_val);
    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = handle_cccd;
    p_msg->req.write_req.gattc_params.len      = WRITE_MESSAGE_LENGTH;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    p_msg->req.write_req.gattc_value[0]        = LSB(cccd_val);
    p_msg->req.write_req.gattc_value[1]        = MSB(cccd_val);
    p_msg->conn_handle                         = conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    
	  return NRF_SUCCESS;
}

uint32_t ble_bp_c_cuff_notif_enable(ble_bp_c_t * p_ble_bp_c)
{
	//bp_log(">>>>ble_bp_c_cuff_notif_enable>>>\r\n");
	 return NOTIFICATION_cccd_configure(p_ble_bp_c->conn_handle, p_ble_bp_c->bp_CUFF_cccd_handle, true);
}

uint32_t ble_bp_c_mea_notif_enable(ble_bp_c_t * p_ble_bp_c)
{
	bp_log(">>>>ble_bp_c_mea_notif_enable>>>\r\n");
	 return INDICATION_cccd_configure(p_ble_bp_c->conn_handle, p_ble_bp_c->bp_MEA_cccd_handle, true);
}
uint32_t ble_bp_c_test_notif_enable(ble_bp_c_t * p_ble_bp_c)
{
	//printf(">>>>>>>>>>>>>>>>.do ble_bp_c_test_notif_enable>>>>>>>>>>\r\n");
	/*
    if (p_ble_bp_c == NULL)
    {
			printf("ble_bp_c_test_notif_enable: p_ble_bp_c=NULL\r\n");
        return NRF_ERROR_NULL;
    }
    printf("call ble_bp_c_test_notif_enable now\r\n");
    return cccd_configure(p_ble_bp_c->conn_handle, p_ble_bp_c->bp_test_cccd_handle, true);
	
*/
	return 0;
}


static void on_write_rsp(ble_bp_c_t * p_bas_c, const ble_evt_t * p_ble_evt)
{
    // Check if there is any message to be sent across to the peer and send it.
    tx_buffer_process();
}


void ble_bp_c_on_ble_evt(ble_bp_c_t * p_ble_bp_c, ble_evt_t * p_ble_evt)
//void ble_bp_on_ble_evt(ble_bp_t * p_bp, ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
					bp_log("ble_bp_on_ble_evt: BLE_GAP_EVT_CONNECTED\r\n");
				  p_ble_bp_c->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
				{
           bp_log("ble_bp_on_ble_evt: BLE_GAP_EVT_DISCONNECTED\r\n");
					 ble_bp_c_evt_t ble_bp_c_evt;
           ble_bp_c_evt.evt_type = BLE_BP_C_EVT_DISCONNECTED;
			    bp_c_evt_handler(p_ble_bp_c, &ble_bp_c_evt);
				}
            break;

        case BLE_GATTC_EVT_HVX:
           bp_log("ble_bp_on_ble_evt: BLE_GATTC_EVT_HVX\r\n");
				   on_hvx(p_ble_bp_c, p_ble_evt);
            break;

        case BLE_GATTC_EVT_WRITE_RSP:
             bp_log("ble_bp_on_ble_evt: BLE_GATTC_EVT_WRITE_RSP\r\n");
				     on_write_rsp(p_ble_bp_c, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


static void db_discover_evt_handler(ble_db_discovery_evt_t * p_evt)
{
	//printf("%d\r\n",__LINE__);
  //printf("bp: db_discover_evt_handler\r\n");
	 // Check if the Heart Rate Service was discovered.
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == /*BLE_UUID_HEART_RATE_SERVICE*/BLE_UUID_BLOOD_PRESSURE_SERVICE &&
        p_evt->params.discovered_db.srv_uuid.type == BLE_UUID_TYPE_BLE)
    {
			//printf("BP: Got db_discover_evt_handler for BP\r\n");
        //mp_ble_hrs_c->conn_handle = p_evt->conn_handle;

        // Find the CCCD Handle of the Heart Rate Measurement characteristic.
        uint32_t i;
			 for (i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
					//BLE_UUID_INTERMEDIATE_CUFF_PRESSURE_CHAR 0x2a36
					//BLE_UUID_BLOOD_PRESSURE_MEASUREMENT_CHAR 0x2a35
					//BLE_UUID_BLOOD_PRESSURE_FEATURE_CHAR 0x2a49
            if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_INTERMEDIATE_CUFF_PRESSURE_CHAR)
            {
							//printf("BP: Found BLE_UUID_INTERMEDIATE_CUFF_PRESSURE_CHAR characteristic\r\n");
                // Found Heart Rate characteristic. Store CCCD handle and break.
                mp_ble_bp_c->bp_CUFF_cccd_handle =     p_evt->params.discovered_db.charateristics[i].cccd_handle;
                mp_ble_bp_c->bp_CUFF_handle      =    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                break;
            }
						if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_BLOOD_PRESSURE_MEASUREMENT_CHAR)
            {
							//printf("BP: Found BLE_UUID_BLOOD_PRESSURE_MEASUREMENT_CHAR characteristic\r\n");
                // Found Heart Rate characteristic. Store CCCD handle and break.
                mp_ble_bp_c->bp_MEA_cccd_handle =     p_evt->params.discovered_db.charateristics[i].cccd_handle;
                mp_ble_bp_c->bp_MEA_handle      =    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                break;
            }
						if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_BLOOD_PRESSURE_FEATURE_CHAR)
            {
							//printf("BP: Found BLE_UUID_BLOOD_PRESSURE_FEATURE_CHAR characteristic\r\n");
                // Found Heart Rate characteristic. Store CCCD handle and break.
                //mp_ble_bp_c->bp_MEA_cccd_handle =     p_evt->params.discovered_db.charateristics[i].cccd_handle;
                mp_ble_bp_c->bp_FEA_handle      =    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                break;
            }
						
							if (p_evt->params.discovered_db.charateristics[i].characteristic.uuid.uuid ==
                BLE_UUID_HEART_RATE_MEASUREMENT_CHAR)
            {
							//printf("BP: Found BLE_UUID_HEART_RATE_MEASUREMENT_CHAR characteristic\r\n");
                // Found Heart Rate characteristic. Store CCCD handle and break.
                mp_ble_bp_c->bp_test_cccd_handle =     p_evt->params.discovered_db.charateristics[i].cccd_handle;
                mp_ble_bp_c->bp_test_handle      =    p_evt->params.discovered_db.charateristics[i].characteristic.handle_value;
                break;
            }
						
        }
				
				ble_bp_c_evt_t evt;
        evt.evt_type = BLE_BP_C_EVT_DISCOVERY_COMPLETE;
				//printf("BP:set BLE_BP_C_EVT_DISCOVERY_COMPLETE\r\n");
				if (mp_ble_bp_c->evt_handler)
          mp_ble_bp_c->evt_handler(mp_ble_bp_c, &evt);
		}

}




//static ble_bp_c_t               m_ble_bp_c; 
uint32_t ble_bp_c_init(ble_bp_c_t * p_ble_bp_c, ble_bp_c_evt_handler_t bp_callback)
//uint32_t ble_bp_c_init(ble_bp_c_evt_handler_t bp_callback)
{
	  //printf("ble_bp_init\r\n");
    ble_uuid_t bp_uuid;
    bp_uuid.type = BLE_UUID_TYPE_BLE;
    bp_uuid.uuid = BLE_UUID_BLOOD_PRESSURE_SERVICE/*BLE_UUID_HEART_RATE_SERVICE*/;

	  mp_ble_bp_c = p_ble_bp_c;
    mp_ble_bp_c->evt_handler     = bp_callback;
    mp_ble_bp_c->conn_handle     = BLE_CONN_HANDLE_INVALID;
    mp_ble_bp_c->bp_test_cccd_handle = BLE_GATT_HANDLE_INVALID;
	 mp_ble_bp_c->bp_CUFF_cccd_handle = BLE_GATT_HANDLE_INVALID;
	 mp_ble_bp_c->bp_CUFF_handle = BLE_GATT_HANDLE_INVALID;
	 mp_ble_bp_c->bp_FEA_handle = BLE_GATT_HANDLE_INVALID;
	 mp_ble_bp_c->bp_MEA_cccd_handle = BLE_GATT_HANDLE_INVALID;
	mp_ble_bp_c->bp_MEA_handle = BLE_GATT_HANDLE_INVALID;
	mp_ble_bp_c->bp_test_handle = BLE_GATT_HANDLE_INVALID;

	
    return ble_db_discovery_evt_register(&bp_uuid,
                                         db_discover_evt_handler);
}
