/******************************************************************************
 *
 * Module: Port
 *
 * File Name: Port.c
 *
 * Description: Source file for TM4C123GH6PM Microcontroller - Port Driver.
 *
 * Author : Hussein El-Shamy
 ******************************************************************************/

#include "Port.h"
#include "Port_Regs.h"
#include "Det.h"

/* AUTOSAR Version checking between Det and Dio Modules */
#if ((DET_AR_MAJOR_VERSION != PORT_AR_RELEASE_MAJOR_VERSION)\
        || (DET_AR_MINOR_VERSION != PORT_AR_RELEASE_MINOR_VERSION)\
        || (DET_AR_PATCH_VERSION != PORT_AR_RELEASE_PATCH_VERSION))
#error "The AR version of Det.h does not match the expected version"
#endif

STATIC Port_Status PORT_Status = PORT_NOT_INITIALIZED;

STATIC const Port_ConfigChannel *Port_PortChannels = NULL_PTR;

/************************************************************************************
 * Service Name:     Port_Init
 * Sync/Async:       Synchronous
 * Reentrancy:       Non Reentrant
 * Parameters (in):  - ConfigPtr -> Pointer to the configuration set.
 * Parameters (inout): None
 * Parameters (out):   None
 * Return value:       None
 * Description:        Initializes the Port Driver module.
 ************************************************************************************/

void Port_Init(const Port_ConfigType *ConfigPtr)
{
    /************************************************************************************
     *                      [0] THE DECLERATION OF LOCAL VARIABLES
     * [A]Declaring the local variables that used in this function
     ***********************************************************************************/

    uint8 pinIndex = PORTA_PA0; /* used as index in the loop, starting with the first Pin*/

    volatile uint32 *PortGpio_Ptr = NULL_PTR; /* point to the required Port Registers base address */

    /************************************************************************************
     *                          [1] DEVELOPMENT EEROR CHECKING
     * [A]Checking the development error: PORT_E_PARAM_CONFIG
     * [B]And if the error is not exist, change the status of module to initialized status
     * [C]Also use global pointer 'Port_PortChannels' to store the address of the first channel configuration structure
     ***********************************************************************************/

#if (PORT_DEV_ERROR_DETECT == STD_ON)
    /* check if the input configuration pointer is not a NULL_PTR */
    if (NULL_PTR == ConfigPtr)
    {
        Det_ReportError(PORT_MODULE_ID, PORT_INSTANCE_ID, PORT_INIT_SID,
        PORT_E_PARAM_CONFIG);
    }
    else
#endif
    {
        /*
         * Set the module state to initialized and point to the PB configuration structure using a global pointer.
         * This global pointer is global to be used by other functions to read the PB configuration structures
         */
        PORT_Status = PORT_INITIALIZED;
        Port_PortChannels = ConfigPtr->Channel; /* address of the first Channels structure --> Channels[0] */
    }

    /************************************************************************************
     *                          [2] LOOPING TO ALL CHANNELS
     * [A] LOOPING TO ALL CHANNELS to assign the Post-build configurations
     ***********************************************************************************/

    for (pinIndex = PORTA_PA0; pinIndex <= PORT_NUMBER_OF_PORT_PINS; pinIndex++)
    {

        /*************************** START OF LOOPING TO ALL CHANNELS **********************/

        /************************************************************************************
         *                          [3] BASE ADDRESS ASSIGNMENT
         * [A] Assigning the base address of the GPIO port to use in manipulating the registers, lately
         ***********************************************************************************/

        switch (Port_PortChannels[pinIndex].port_num)
        {
        case 0:
            PortGpio_Ptr = (volatile uint32*) GPIO_PORTA_BASE_ADDRESS; /* PORTA Base Address */
            break;
        case 1:
            PortGpio_Ptr = (volatile uint32*) GPIO_PORTB_BASE_ADDRESS; /* PORTB Base Address */
            break;
        case 2:
            PortGpio_Ptr = (volatile uint32*) GPIO_PORTC_BASE_ADDRESS; /* PORTC Base Address */
            break;
        case 3:
            PortGpio_Ptr = (volatile uint32*) GPIO_PORTD_BASE_ADDRESS; /* PORTD Base Address */
            break;
        case 4:
            PortGpio_Ptr = (volatile uint32*) GPIO_PORTE_BASE_ADDRESS; /* PORTE Base Address */
            break;
        case 5:
            PortGpio_Ptr = (volatile uint32*) GPIO_PORTF_BASE_ADDRESS; /* PORTF Base Address */
            break;
        }

        /************************************************************************************
         *                              [4] LOCKING AND JTAG PINs
         * [A] Checking the LOCK and JTAG Pins
         ***********************************************************************************/

        if (((Port_PortChannels[pinIndex].port_num == 3)
                && (Port_PortChannels[pinIndex].pin_num == 7))
                || ((Port_PortChannels[pinIndex].port_num == 5)
                        && (Port_PortChannels[pinIndex].pin_num == 0))) /* PD7 or PF0 */
        {
            *(volatile uint32*) ((volatile uint8*) PortGpio_Ptr
                    + PORT_LOCK_REG_OFFSET) = 0x4C4F434B; /* Unlock the GPIOCR register */
            SET_BIT(*(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_COMMIT_REG_OFFSET),
                    Port_PortChannels[pinIndex].pin_num); /* Set the corresponding bit in GPIOCR register to allow changes on this pin */
        }
        else if ((Port_PortChannels[pinIndex].port_num == 2)
                && (Port_PortChannels[pinIndex].pin_num <= 3)) /* PC0 to PC3 */
        {
            /* Do Nothing ...  this is the JTAG pins */
        }
        else
        {
            /* Do Nothing ... No need to unlock the commit register for this pin */
        }

        /************************************************************************************
         *                          [5] DIRECTION AND INTERNAL RESISTORS
         * [A] Configure the Direction of the pin
         * [B] if the direction configuration is INPUT, Configure the INTERNAL RESISTORS (UP,DOWN,OFF)
         ***********************************************************************************/

        if (Port_PortChannels[pinIndex].direction == OUTPUT)
        {
            SET_BIT(*(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_DIR_REG_OFFSET),
                    Port_PortChannels[pinIndex].pin_num); /* Set the corresponding bit in the GPIODIR register to configure it as output pin */

            if (Port_PortChannels[pinIndex].initial_value == STD_HIGH)
            {
                SET_BIT(*(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_DATA_REG_OFFSET),
                        Port_PortChannels[pinIndex].pin_num); /* Set the corresponding bit in the GPIODATA register to provide initial value 1 */
            }
            else
            {
                CLEAR_BIT(
                        *(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_DATA_REG_OFFSET),
                        Port_PortChannels[pinIndex].pin_num); /* Clear the corresponding bit in the GPIODATA register to provide initial value 0 */
            }
        }
        else if (Port_PortChannels[pinIndex].direction == INPUT)
        {
            CLEAR_BIT(
                    *(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_DIR_REG_OFFSET),
                    Port_PortChannels[pinIndex].pin_num); /* Clear the corresponding bit in the GPIODIR register to configure it as input pin */

            if (Port_PortChannels[pinIndex].resistor == PULL_UP)
            {
                SET_BIT(*(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_PULL_UP_REG_OFFSET),
                        Port_PortChannels[pinIndex].pin_num); /* Set the corresponding bit in the GPIOPUR register to enable the internal pull up pin */
            }
            else if (Port_PortChannels[pinIndex].resistor == PULL_DOWN)
            {
                SET_BIT(*(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_PULL_DOWN_REG_OFFSET),
                        Port_PortChannels[pinIndex].pin_num); /* Set the corresponding bit in the GPIOPDR register to enable the internal pull down pin */
            }
            else
            {
                CLEAR_BIT(
                        *(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_PULL_UP_REG_OFFSET),
                        Port_PortChannels[pinIndex].pin_num); /* Clear the corresponding bit in the GPIOPUR register to disable the internal pull up pin */
                CLEAR_BIT(
                        *(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_PULL_DOWN_REG_OFFSET),
                        Port_PortChannels[pinIndex].pin_num); /* Clear the corresponding bit in the GPIOPDR register to disable the internal pull down pin */
            }
        }
        else
        {
            /* Do Nothing */
        }

        /************************************************************************************
         *                      [6] ANALOG AND DIGITAL FUNCTIONALITY OF THE PIN
         * [A] ENABLE or DISABLE the analog and digital functionality of the pin based on the PB configuration
         ***********************************************************************************/

        if (PORT_PIN_MODE_ADC == Port_PortChannels[pinIndex].pin_mode)
        {
            /************************************ INCASE OF THE ADC  *****************************************/

            /************************** ENABLE THE ANALOG FUNCTIONALITY **************************************
             * Set the corresponding bit in the GPIOAMSEL register to disable analog functionality on this pin*
             *************************************************************************************************/
            SET_BIT(*(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_ANALOG_MODE_SEL_REG_OFFSET),
                    Port_PortChannels[pinIndex].pin_num);

            /************************** DISABLE THE DIGITAL FUNCTIONALITY *************************************
             *  Set the corresponding bit in the GPIODEN register to enable digital functionality on this pin  *
             ***************************************************************************************************/
            CLEAR_BIT(
                    *(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_DIGITAL_ENABLE_REG_OFFSET),
                    Port_PortChannels[pinIndex].pin_num);
        }
        else
        {
            /************************** DISABLE THE ANALOG FUNCTIONALITY **************************************
             *Clear the corresponding bit in the GPIOAMSEL register to disable analog functionality on this pin*
             ***************************************************************************************************/
            CLEAR_BIT(
                    *(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_ANALOG_MODE_SEL_REG_OFFSET),
                    Port_PortChannels[pinIndex].pin_num);

            /************************** ENABLE THE DIGITAL FUNCTIONALITY *************************************
             * Set the corresponding bit in the GPIODEN register to enable digital functionality on this pin  *
             **************************************************************************************************/
            SET_BIT(*(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_DIGITAL_ENABLE_REG_OFFSET),
                    Port_PortChannels[pinIndex].pin_num);
        }

        /************************************************************************************
         *          [7] ALTERNATIVE FUNCTIONALITY AND CONTROL REGISTER PCMx OF THE PIN
         * [A] ENABLE or DISABLE the alternative functionality of the pin based on the PB configuration
         * [B] IF ENABLE, So Assigning the value of PCMx REGISTER based on the mode of the pin configured in PB structure
         ***********************************************************************************/

        if (PORT_PIN_MODE_DIO == Port_PortChannels[pinIndex].pin_mode)
        {
            /************************************* DISABLE ***************************************/
            /* Disable Alternative function for this pin by clear the corresponding bit in GPIOAFSEL register */
            CLEAR_BIT(
                    *(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_ALT_FUNC_REG_OFFSET),
                    Port_PortChannels[pinIndex].pin_num);
        }
        else
        {
            /************************************* ENABLE ***************************************/
            /* Enable Alternative function for this pin by clear the corresponding bit in GPIOAFSEL register */
            SET_BIT(*(volatile uint32 *)((volatile uint8 *)PortGpio_Ptr + PORT_ALT_FUNC_REG_OFFSET),
                    Port_PortChannels[pinIndex].pin_num);
            /************************************************************************************
             * [B] Assigning the value of PCMx register based on the mode of the pin configured in PB structure
             ***********************************************************************************/

            switch (Port_PortChannels[pinIndex].pin_mode)
            {

            /****************************************** CAN *************************************/
            case PORT_PIN_MODE_CAN:

                if (Port_PortChannels[pinIndex].port_num == PORT_F
                        && (Port_PortChannels[pinIndex].pin_num == PIN0_PIN_NUM
                                || Port_PortChannels[pinIndex].pin_num
                                        == PIN3_PIN_NUM ))
                {
                    /************************************************************************
                     *                         with PF0 & PF3 >>> PCMx = 3
                     ***********************************************************************/

                    *(volatile uint32*) ((volatile uint8*) PortGpio_Ptr
                            + PORT_CTL_REG_OFFSET) |= (0x00000003
                            << (Port_PortChannels[pinIndex].pin_num * 4));

                }
                else
                {
                    /************************************************************************
                     *          with PA0 & PA1 & PB4 & PB5 & PE4 & PE5 >>> PCMx = 8
                     ***********************************************************************/

                    *(volatile uint32*) ((volatile uint8*) PortGpio_Ptr
                            + PORT_CTL_REG_OFFSET) |= (0x00000008
                            << (Port_PortChannels[pinIndex].pin_num * 4));

                }

                break;
                /****************************************** GPT *************************************/
            case PORT_PIN_MODE_GPT:

                *(volatile uint32*) ((volatile uint8*) PortGpio_Ptr
                        + PORT_CTL_REG_OFFSET) |= (0x00000007
                        << (Port_PortChannels[pinIndex].pin_num * 4));

                break;
                /****************************************** I2C *************************************/
            case PORT_PIN_MODE_I2C:

                *(volatile uint32*) ((volatile uint8*) PortGpio_Ptr
                        + PORT_CTL_REG_OFFSET) |= (0x00000003
                        << (Port_PortChannels[pinIndex].pin_num * 4));

                break;

                /****************************************** PWM *************************************/
            case PORT_PIN_MODE_PWM:

                /* NOTE: The SW only support the Motion Control Module 0 */

                *(volatile uint32*) ((volatile uint8*) PortGpio_Ptr
                        + PORT_CTL_REG_OFFSET) |= (0x00000004
                        << (Port_PortChannels[pinIndex].pin_num * 4));

                break;

                /****************************************** SSI *************************************/
            case PORT_PIN_MODE_SSI:

                if (Port_PortChannels[pinIndex].port_num
                        == PORT_D&& Port_PortChannels[pinIndex].pin_num <=PIN3_PIN_NUM)
                {
                    /************************************************************************
                     *                         with PD0 & PD1 & PD2 & PD3 >>> PCMx = 1
                     ***********************************************************************/

                    *(volatile uint32*) ((volatile uint8*) PortGpio_Ptr
                            + PORT_CTL_REG_OFFSET) |= (0x00000001
                            << (Port_PortChannels[pinIndex].pin_num * 4));

                }
                else
                {
                    /************************************************************************
                     *                         with other pins
                     ***********************************************************************/
                    *(volatile uint32*) ((volatile uint8*) PortGpio_Ptr
                            + PORT_CTL_REG_OFFSET) |= (0x00000002
                            << (Port_PortChannels[pinIndex].pin_num * 4));
                }

                break;

                /****************************************** UART *************************************/
            case PORT_PIN_MODE_UART:

                *(volatile uint32*) ((volatile uint8*) PortGpio_Ptr
                        + PORT_CTL_REG_OFFSET) |= (0x00000001
                        << (Port_PortChannels[pinIndex].pin_num * 4));

                break;

            default:

                /* Do nothing */

                break;

                /**************************END OF SWITCH **********************************************/
            }

            /**************************END OF IF CONDITION **********************************************/
        }

        /*************************** END OF LOOPING TO ALL CHANNELS ************************/
    }

    /*************************** END OF THE INTIALZATION FUNCTION **********************/
}

#if (PORT_SET_PIN_DIRECTION_API == STD_ON)
/************************************************************************************
 * Service Name:     Port_SetPinDirection
 * Sync/Async:       Synchronous
 * Reentrancy:       Reentrant
 * Parameters (in):  - Pin       -> Port Pin ID number.
 *                   - Direction -> Port Pin Direction.
 * Parameters (inout): None
 * Parameters (out):   None
 * Return value:       None
 * Description:        Sets the port pin direction.
 ************************************************************************************/
void Port_SetPinDirection(Port_PinType Pin, Port_PinDirectionType Direction)
{

}
#endif

/************************************************************************************
 * Service Name:     Port_RefreshPortDirection
 * Sync/Async:       Synchronous
 * Reentrancy:       Non Reentrant
 * Parameters (in):  None
 * Parameters (inout): None
 * Parameters (out):   None
 * Return value:       None
 * Description:        Refreshes port direction.
 ************************************************************************************/
void Port_RefreshPortDirection(void)
{

}

#if (PORT_VERSION_INFO_API==STD_ON)
/************************************************************************************
 * Service Name:     Port_GetVersionInfo
 * Sync/Async:       Synchronous
 * Reentrancy:       Non Reentrant
 * Parameters (in):  None
 * Parameters (inout): None
 * Parameters (out):   versioninfo -> Pointer to where to store the version information
 * Return value:       None
 * Description:        Returns the version information of this module.
 ************************************************************************************/
void Port_GetVersionInfo(Std_VersionInfoType *versioninfo)
{

}

#endif

#if (PORT_SET_PIN_MODE_API==STD_ON)
/************************************************************************************
 * Service Name:     Port_SetPinMode
 * Sync/Async:       Synchronous
 * Reentrancy:       Reentrant
 * Parameters (in):  - Pin  -> Port Pin ID number.
 *                   - Mode -> New Port Pin mode to be set on port pin.
 * Parameters (inout): None
 * Parameters (out):   None
 * Return value:       None
 * Description:        This function sets the port pin mode:
 *                     - It modifies the mode of the specified port pin during runtime.
 ************************************************************************************/
void Port_SetPinMode(Port_PinType Pin, Port_PinModeType Mode)
{

}
#endif
