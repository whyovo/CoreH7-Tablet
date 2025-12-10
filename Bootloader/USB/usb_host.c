/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file            : usb_host.c
 * @version         : v1.0_Cube
 * @brief           : This file implements the USB Host
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_hid.h"
#include "usbh_hid_mouse.h"
#include "usbh_hid_keybd.h"
#ifdef USB_HOST_ENABLE /*!< USB HOST驱动使能 */
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USB Host core handle declaration */
USBH_HandleTypeDef hUsbHostHS;
ApplicationTypeDef Appli_state = APPLICATION_IDLE;

/*
 * -- Insert your variables declaration here --
 */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*
 * user callback declaration
 */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);

/*
 * -- Insert your external function declaration here --
 */
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
 * Init USB host library, add supported class and start the library
 * @retval None
 */
void MX_USB_HOST_Init(void)
{
  /* USER CODE BEGIN USB_HOST_Init_PreTreatment */

  /* USER CODE END USB_HOST_Init_PreTreatment */

  /* Init host Library, add supported class and start the library. */
  if (USBH_Init(&hUsbHostHS, USBH_UserProcess, HOST_HS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_RegisterClass(&hUsbHostHS, USBH_HID_CLASS) != USBH_OK)
  {
    Error_Handler();
  }
  if (USBH_Start(&hUsbHostHS) != USBH_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_HOST_Init_PostTreatment */

  /* USER CODE END USB_HOST_Init_PostTreatment */
}

/*
 * user callback definition
 */
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
  /* USER CODE BEGIN CALL_BACK_1 */
  switch (id)
  {
  case HOST_USER_SELECT_CONFIGURATION:
    break;

  case HOST_USER_DISCONNECTION:
    Appli_state = APPLICATION_DISCONNECT;
    DEBUG_INFO("USB设备已断开连接");
    break;

  case HOST_USER_CLASS_ACTIVE:
    Appli_state = APPLICATION_READY;
    {
      HID_TypeTypeDef device_type = USBH_HID_GetDeviceType(phost);
      if (device_type == HID_MOUSE)
      {
        DEBUG_INFO("鼠标已连接");
      }
      else if (device_type == HID_KEYBOARD)
      {
        DEBUG_INFO("键盘已连接");
      }
    }
    break;

  case HOST_USER_CONNECTION:
    Appli_state = APPLICATION_START;
    DEBUG_INFO("USB设备已连接");
    break;

  default:
    break;
  }
  /* USER CODE END CALL_BACK_1 */
}

/**
 * Background task with HID data processing
 */
void MX_USB_HOST_Process(void)
{
  /* USB Host Background task */
  USBH_Process(&hUsbHostHS);

  /* Process HID data if device is ready */
  if (Appli_state == APPLICATION_READY)
  {
    static char debug_buffer[128];
    HID_TypeTypeDef device_type = USBH_HID_GetDeviceType(&hUsbHostHS);

    if (device_type == HID_MOUSE)
    {
      HID_MOUSE_Info_TypeDef *mouse_info = USBH_HID_GetMouseInfo(&hUsbHostHS);
      if (mouse_info != NULL)
      {
        snprintf(debug_buffer, sizeof(debug_buffer),
                 "鼠标 - X:%d Y:%d 按键:%d %d %d",
                 mouse_info->x,
                 mouse_info->y,
                 mouse_info->buttons[0],
                 mouse_info->buttons[1],
                 mouse_info->buttons[2]);
        DEBUG_INFO(debug_buffer);
      }
    }
    else if (device_type == HID_KEYBOARD)
    {
      HID_KEYBD_Info_TypeDef *kbd_info = USBH_HID_GetKeybdInfo(&hUsbHostHS);
      if (kbd_info != NULL && kbd_info->keys[0] != 0)
      {
        uint8_t ascii_code = USBH_HID_GetASCIICode(kbd_info);
        snprintf(debug_buffer, sizeof(debug_buffer),
                 "键盘 - 按键码:0x%02X ASCII:%c Ctrl:%d Shift:%d Alt:%d",
                 kbd_info->keys[0],
                 (ascii_code >= 32 && ascii_code < 127) ? ascii_code : '?',
                 kbd_info->lctrl || kbd_info->rctrl,
                 kbd_info->lshift || kbd_info->rshift,
                 kbd_info->lalt || kbd_info->ralt);
        DEBUG_INFO(debug_buffer);
      }
    }
  }
}

/**
 * @}
 */

/**
 * @}
 */
#endif
