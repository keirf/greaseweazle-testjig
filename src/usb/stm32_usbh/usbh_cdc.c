/******************************************************************************
*
* <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
*
* Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
* You may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
*        http://www.st.com/software_license_agreement_liberty_v2
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
******************************************************************************
*/

#include "usbh_cdc.h"

CDC_HandleTypeDef _CDC_Handle;

static USBH_Status USBH_CDC_InterfaceInit  (USB_OTG_CORE_HANDLE *pdev ,
                                            void *phost);

static void USBH_CDC_InterfaceDeInit  (USB_OTG_CORE_HANDLE *pdev ,
                                       void *phost);

static USBH_Status USBH_CDC_Handle(USB_OTG_CORE_HANDLE *pdev ,

                                   void *phost);

static USBH_Status USBH_CDC_ClassRequest(USB_OTG_CORE_HANDLE *pdev ,
                                         void *phost);

static void CDC_ProcessTransmission(USB_OTG_CORE_HANDLE *pdev ,
                                    void   *phost);
static void CDC_ProcessReception(USB_OTG_CORE_HANDLE *pdev ,
                                 void   *phost);

USBH_Class_cb_TypeDef  USBH_CDC_cb =
{
    USBH_CDC_InterfaceInit,
    USBH_CDC_InterfaceDeInit,
    USBH_CDC_ClassRequest,
    USBH_CDC_Handle,
};

/**
 * @brief  USBH_CDC_InterfaceInit
 *         Interface initialization for CDC class.
 * @param  pdev: Selected device
 * @param  hdev: Selected device property
 * @retval USBH_Status : Status of class request handled.
 */
static USBH_Status USBH_CDC_InterfaceInit ( USB_OTG_CORE_HANDLE *pdev,
                                            void *phost)
{	
    USBH_HOST *pphost = phost;

    if((pphost->device_prop.Itf_Desc[0].bInterfaceClass != USB_CDC_CLASS) ||
            (pphost->device_prop.Itf_Desc[0].bInterfaceProtocol != COMMON_AT_COMMAND))
    {
        printk("Attached device %02x:%02x is not supported\n",
               pphost->device_prop.Itf_Desc[0].bInterfaceClass,
               pphost->device_prop.Itf_Desc[0].bInterfaceProtocol);

        pphost->usr_cb->DeviceNotSupported();
        return USBH_OK ;
    }

    if(pphost->device_prop.Ep_Desc[1][0].bEndpointAddress & 0x80)
    {
        _CDC_Handle.DataItf.InEp = (pphost->device_prop.Ep_Desc[1][0].bEndpointAddress);
        _CDC_Handle.DataItf.InEpSize  = pphost->device_prop.Ep_Desc[1][0].wMaxPacketSize;
    }
    else
    {
        _CDC_Handle.DataItf.OutEp = (pphost->device_prop.Ep_Desc[1][0].bEndpointAddress);
        _CDC_Handle.DataItf.OutEpSize  = pphost->device_prop.Ep_Desc[1][0].wMaxPacketSize;
    }

    if(pphost->device_prop.Ep_Desc[1][1].bEndpointAddress & 0x80)
    {
        _CDC_Handle.DataItf.InEp = (pphost->device_prop.Ep_Desc[1][1].bEndpointAddress);
        _CDC_Handle.DataItf.InEpSize  = pphost->device_prop.Ep_Desc[1][1].wMaxPacketSize;
    }
    else
    {
        _CDC_Handle.DataItf.OutEp = (pphost->device_prop.Ep_Desc[1][1].bEndpointAddress);
        _CDC_Handle.DataItf.OutEpSize  = pphost->device_prop.Ep_Desc[1][1].wMaxPacketSize;
    }

    _CDC_Handle.DataItf.OutPipe = USBH_Alloc_Channel(pdev,
                                                _CDC_Handle.DataItf.OutEp);
    _CDC_Handle.DataItf.InPipe = USBH_Alloc_Channel(pdev,
                                               _CDC_Handle.DataItf.InEp);

    /* Open the new channels */
    USBH_Open_Channel  (pdev,
                        _CDC_Handle.DataItf.OutPipe,
                        pphost->device_prop.address,
                        pphost->device_prop.speed,
                        EP_TYPE_BULK,
                        _CDC_Handle.DataItf.OutEpSize);

    USBH_Open_Channel  (pdev,
                        _CDC_Handle.DataItf.InPipe,
                        pphost->device_prop.address,
                        pphost->device_prop.speed,
                        EP_TYPE_BULK,
                        _CDC_Handle.DataItf.InEpSize);
    return USBH_OK;
}


/**
 * @brief  USBH_CDC_InterfaceDeInit
 *         De-Initialize interface by freeing host channels allocated to interface
 * @param  pdev: Selected device
 * @param  hdev: Selected device property
 * @retval None
 */
void USBH_CDC_InterfaceDeInit ( USB_OTG_CORE_HANDLE *pdev,
                                void *phost)
{	
    if ( _CDC_Handle.DataItf.OutPipe)
    {
        USB_OTG_HC_Halt(pdev, _CDC_Handle.DataItf.OutPipe);
        USBH_Free_Channel  (pdev, _CDC_Handle.DataItf.OutPipe);
        _CDC_Handle.DataItf.OutPipe = 0;     /* Reset the Channel as Free */
    }

    if ( _CDC_Handle.DataItf.InPipe)
    {
        USB_OTG_HC_Halt(pdev, _CDC_Handle.DataItf.InPipe);
        USBH_Free_Channel  (pdev, _CDC_Handle.DataItf.InPipe);
        _CDC_Handle.DataItf.InPipe = 0;     /* Reset the Channel as Free */
    }
}

/**
 * @brief  USBH_CDC_ClassRequest
 *         This function will only initialize the CDC state machine
 * @param  pdev: Selected device
 * @param  hdev: Selected device property
 * @retval USBH_Status : Status of class request handled.
 */

static USBH_Status USBH_CDC_ClassRequest(USB_OTG_CORE_HANDLE *pdev ,
                                         void *phost)
{
    USBH_HOST *pphost = phost;
    USBH_Status status = USBH_OK ;
    CDC_HandleTypeDef *CDC_Handle = (CDC_HandleTypeDef *)&_CDC_Handle;

    CDC_Handle->state = CDC_IDLE_STATE;
    pphost->usr_cb->UserApplication();
    return status;
}


/**
 * @brief  USBH_CDC_Handle
 *         CDC state machine handler
 * @param  pdev: Selected device
 * @param  hdev: Selected device property
 * @retval USBH_Status
 */

static USBH_Status USBH_CDC_Handle(USB_OTG_CORE_HANDLE *pdev ,
                                   void   *phost)
{
    CDC_HandleTypeDef *CDC_Handle = (CDC_HandleTypeDef *)&_CDC_Handle;
    USBH_Status status = USBH_BUSY;

    if(HCD_IsDeviceConnected(pdev))
    {
        switch(CDC_Handle->state)
        {

        case CDC_IDLE_STATE:
            status = USBH_OK;
            break;

        case CDC_TRANSFER_DATA:
            CDC_ProcessTransmission(pdev, phost);
            CDC_ProcessReception(pdev, phost);
            break;

        default:
            break;

        }
    }
    return status;
}

/**
  * @brief  This function prepares the state before issuing the class specific commands
  * @param  None
  * @retval None
  */
USBH_Status  USBH_CDC_Transmit(uint8_t *pbuff, uint32_t length)
{
  USBH_Status Status = USBH_BUSY;
  CDC_HandleTypeDef *CDC_Handle = (CDC_HandleTypeDef *)&_CDC_Handle;

  if ((CDC_Handle->state == CDC_IDLE_STATE) || (CDC_Handle->state == CDC_TRANSFER_DATA))
  {
    CDC_Handle->pTxData = pbuff;
    CDC_Handle->TxDataLength = length;
    CDC_Handle->state = CDC_TRANSFER_DATA;
    CDC_Handle->data_tx_state = CDC_SEND_DATA;
    Status = USBH_OK;

  }
  return Status;
}


/**
* @brief  This function prepares the state before issuing the class specific commands
* @param  None
* @retval None
*/
USBH_Status  USBH_CDC_Receive(uint8_t *pbuff, uint32_t length)
{
  USBH_Status Status = USBH_BUSY;
  CDC_HandleTypeDef *CDC_Handle = (CDC_HandleTypeDef *)&_CDC_Handle;

  if ((CDC_Handle->state == CDC_IDLE_STATE) || (CDC_Handle->state == CDC_TRANSFER_DATA))
  {
    CDC_Handle->pRxData = pbuff;
    CDC_Handle->RxDataLength = length;
    CDC_Handle->state = CDC_TRANSFER_DATA;
    CDC_Handle->data_rx_state = CDC_RECEIVE_DATA;
    Status = USBH_OK;

  }
  return Status;
}

/**
* @brief  The function is responsible for sending data to the device
*  @param  pdev: Selected device
* @retval None
*/
static void CDC_ProcessTransmission(USB_OTG_CORE_HANDLE *pdev ,
                                   void   *phost)
{
    CDC_HandleTypeDef *CDC_Handle = (CDC_HandleTypeDef *)&_CDC_Handle;
    URB_STATE URB_Status = URB_IDLE;

  switch (CDC_Handle->data_tx_state)
  {
    case CDC_SEND_DATA:
      if (CDC_Handle->TxDataLength > CDC_Handle->DataItf.OutEpSize)
      {
        USBH_BulkSendData(pdev,
                          CDC_Handle->pTxData,
                          CDC_Handle->DataItf.OutEpSize,
                          CDC_Handle->DataItf.OutPipe);
      }
      else
      {
        USBH_BulkSendData(pdev,
                          CDC_Handle->pTxData,
                          (uint16_t)CDC_Handle->TxDataLength,
                          CDC_Handle->DataItf.OutPipe);
      }

      CDC_Handle->data_tx_state = CDC_SEND_DATA_WAIT;
      break;

    case CDC_SEND_DATA_WAIT:

      URB_Status = HCD_GetURB_State(pdev, CDC_Handle->DataItf.OutPipe);

      /* Check the status done for transmission */
      if (URB_Status == URB_DONE)
      {
        if (CDC_Handle->TxDataLength > CDC_Handle->DataItf.OutEpSize)
        {
          CDC_Handle->TxDataLength -= CDC_Handle->DataItf.OutEpSize;
          CDC_Handle->pTxData += CDC_Handle->DataItf.OutEpSize;
        }
        else
        {
          CDC_Handle->TxDataLength = 0U;
        }

        if (CDC_Handle->TxDataLength > 0U)
        {
          CDC_Handle->data_tx_state = CDC_SEND_DATA;
        }
        else
        {
          CDC_Handle->data_tx_state = CDC_IDLE;
          USBH_CDC_TransmitCallback();
        }

      }
      else
      {
        if (URB_Status == URB_NOTREADY)
        {
          CDC_Handle->data_tx_state = CDC_SEND_DATA;

        }
      }
      break;

    default:
      break;
  }
}
/**
* @brief  This function responsible for reception of data from the device
*  @param  pdev: Selected device
* @retval None
*/

static void CDC_ProcessReception(USB_OTG_CORE_HANDLE *pdev ,
                                 void   *phost)
{
    CDC_HandleTypeDef *CDC_Handle = (CDC_HandleTypeDef *)&_CDC_Handle;
  URB_STATE URB_Status = URB_IDLE;
  uint32_t length;

  switch (CDC_Handle->data_rx_state)
  {

    case CDC_RECEIVE_DATA:

      USBH_BulkReceiveData(pdev,
                           CDC_Handle->pRxData,
                           CDC_Handle->DataItf.InEpSize,
                           CDC_Handle->DataItf.InPipe);

      CDC_Handle->data_rx_state = CDC_RECEIVE_DATA_WAIT;

      break;

    case CDC_RECEIVE_DATA_WAIT:

      URB_Status = HCD_GetURB_State(pdev, CDC_Handle->DataItf.InPipe);

      /*Check the status done for reception*/
      if (URB_Status == URB_DONE)
      {
        length = HCD_GetXferCnt(pdev, CDC_Handle->DataItf.InPipe);

        if (length == 0)
        {
          CDC_Handle->data_rx_state = CDC_RECEIVE_DATA;
        }
        else
        {
          CDC_Handle->data_rx_state = CDC_IDLE;
          USBH_CDC_ReceiveCallback();
        }

      }
      break;

    default:
      break;
  }
}
