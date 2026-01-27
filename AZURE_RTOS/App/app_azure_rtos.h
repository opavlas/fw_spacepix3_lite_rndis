/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_azure_rtos.h
  * @author  MCD Application Team
  * @brief   app_azure_rtos application header file
  ******************************************************************************
   * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef APP_AZURE_RTOS_H
#define APP_AZURE_RTOS_H
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "app_threadx.h"
#include "stm32u5xx_hal.h"
#include "app_azure_rtos_config.h"

#include "app_netxduo.h"
#include "app_usbx_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define USE_STATIC_ALLOCATION                    1

#define TX_APP_MEM_POOL_SIZE                     8192

#define FX_APP_MEM_POOL_SIZE                     1

#define NX_APP_MEM_POOL_SIZE                     1500*1024

#define UX_DEVICE_APP_MEM_POOL_SIZE              40*1024
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

#ifdef __cplusplus
}
#endif
#endif /* APP_AZURE_RTOS_H */
