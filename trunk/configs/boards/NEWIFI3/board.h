/* NEWIFI3(d2) */

#define BOARD_PID		"NEWIFI3"
#define BOARD_NAME		"NEWIFI3"
#define BOARD_DESC		"NEWIFI3 Wireless Router"
#define BOARD_VENDOR_NAME	"Diting Technology"
#define BOARD_VENDOR_URL	"http://www.newifi.com/"
#define BOARD_MODEL_URL		"http://www.newifi.com/"
#define BOARD_BOOT_TIME		30
#define BOARD_FLASH_TIME	120
#undef BOARD_GPIO_BTN_FN1
#define BOARD_GPIO_BTN_RESET	3
#define BOARD_GPIO_BTN_WPS	7
#undef  BOARD_GPIO_LED_ALL
#undef  BOARD_GPIO_LED_WIFI
#define  BOARD_GPIO_LED_SW2G	14
#define  BOARD_GPIO_LED_SW5G	16
#define  BOARD_GPIO_LED_POWER	15		/* sys_blue: 15 */
#define  BOARD_GPIO_LED_POWER2	6		/* sys_purple: 6 */
#undef  BOARD_GPIO_LED_LAN
#define BOARD_GPIO_LED_WAN	13		/* wan_blue: 13 */
#define BOARD_GPIO_LED_WAN2	4		/* wan_purple: 4 */
#define BOARD_GPIO_LED_USB	10
#undef BOARD_GPIO_LED_ROUTER
#define BOARD_HAS_5G_11AC	1
#define BOARD_NUM_ANT_5G_TX	2
#define BOARD_NUM_ANT_5G_RX	2
#define BOARD_NUM_ANT_2G_TX	2
#define BOARD_NUM_ANT_2G_RX	2
#define BOARD_NUM_ETH_LEDS	1
#define BOARD_HAS_EPHY_L1000	1
#define BOARD_HAS_EPHY_W1000	1
#define BOARD_NUM_UPHY_USB3	1
#define BOARD_GPIO_PWR_USB	11
#define BOARD_GPIO_PWR_USB2	9
#define BOARD_GPIO_PWR_USB_ON	1
#define BOARD_USB_PORT_SWAP	0
