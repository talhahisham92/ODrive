/*
* @brief Contains board specific variables and initialization functions
*/

#include <board.h>

#include <odrive_main.h>
#include <low_level.h>

#include <Drivers/STM32/stm32_timer.hpp>
#include <Drivers/STM32/stm32_usart.hpp>
#include <Drivers/STM32/stm32_spi.hpp>
#include <Drivers/STM32/stm32_can.hpp>
#include <Drivers/STM32/stm32_basic_pwm_output.hpp>

#include <adc.h>
#include <tim.h>
#include <freertos_vars.h>

// this should technically be in task_timer.cpp but let's not make a one-line file
bool TaskTimer::enabled = false;

extern "C" void SystemClock_Config(void); // defined in main.c generated by CubeMX

#define ControlLoop_IRQHandler OTG_HS_IRQHandler
#define ControlLoop_IRQn OTG_HS_IRQn

// This array is placed at the very start of the ram (0x20000000) and will be
// used during manufacturing to test the struct that will go to the OTP before
// _actually_ putting anything into OTP. This avoids bulk-destroying STM32's if
// we introduce unintended breakage in our manufacturing scripts.
uint8_t __attribute__((section(".testdata"))) fake_otp[FLASH_OTP_END + 1 - FLASH_OTP_BASE];


/* Internal GPIOs ------------------------------------------------------------*/

Stm32Gpio drv0_ncs_gpio = {GPIOC, GPIO_PIN_13};
Stm32Gpio drv1_ncs_gpio = {GPIOC, GPIO_PIN_14};
Stm32Gpio drv_nfault_gpio = {GPIOD, GPIO_PIN_2};
Stm32Gpio drv_en_gate_gpio = {GPIOB, GPIO_PIN_12};

Stm32Gpio spi_miso_gpio = {GPIOC, GPIO_PIN_11};
Stm32Gpio spi_mosi_gpio = {GPIOC, GPIO_PIN_12};
Stm32Gpio spi_clk_gpio = {GPIOC, GPIO_PIN_10};

#if (HW_VERSION_MINOR == 1) || (HW_VERSION_MINOR == 2)
Stm32Gpio vbus_s_gpio = {GPIOA, GPIO_PIN_0};
Stm32Gpio aux_fet_temp_gpio = {GPIOC, GPIO_PIN_4};
Stm32Gpio m0_fet_temp_gpio = {GPIOC, GPIO_PIN_5};
Stm32Gpio m1_fet_temp_gpio = {GPIOA, GPIO_PIN_1};
#elif (HW_VERSION_MINOR == 3) || (HW_VERSION_MINOR == 4)
Stm32Gpio vbus_s_gpio = {GPIOA, GPIO_PIN_6};
Stm32Gpio aux_fet_temp_gpio = {GPIOC, GPIO_PIN_4};
Stm32Gpio m0_fet_temp_gpio = {GPIOC, GPIO_PIN_5};
Stm32Gpio m1_fet_temp_gpio = {GPIOA, GPIO_PIN_4};
#elif (HW_VERSION_MINOR == 5) || (HW_VERSION_MINOR == 6)
Stm32Gpio vbus_s_gpio = {GPIOA, GPIO_PIN_6};
Stm32Gpio aux_fet_temp_gpio = {GPIOA, GPIO_PIN_5};
Stm32Gpio m0_fet_temp_gpio = {GPIOC, GPIO_PIN_5};
Stm32Gpio m1_fet_temp_gpio = {GPIOA, GPIO_PIN_4};
#endif

Stm32Gpio m0_sob_gpio = {GPIOC, GPIO_PIN_0};
Stm32Gpio m0_soc_gpio = {GPIOC, GPIO_PIN_1};
Stm32Gpio m1_sob_gpio = {GPIOC, GPIO_PIN_3};
Stm32Gpio m1_soc_gpio = {GPIOC, GPIO_PIN_2};


/* External GPIOs ------------------------------------------------------------*/

#if (HW_VERSION_MINOR == 1) || (HW_VERSION_MINOR == 2)
Stm32Gpio gpios[] = {
    {nullptr, 0}, // dummy GPIO0 so that PCB labels and software numbers match

    {GPIOB, GPIO_PIN_2}, // GPIO1
    {GPIOA, GPIO_PIN_5}, // GPIO2
    {GPIOA, GPIO_PIN_4}, // GPIO3
    {GPIOA, GPIO_PIN_3}, // GPIO4
    {nullptr, 0}, // GPIO5 (doesn't exist on this board)
    {nullptr, 0}, // GPIO6 (doesn't exist on this board)
    {nullptr, 0}, // GPIO7 (doesn't exist on this board)
    {nullptr, 0}, // GPIO8 (doesn't exist on this board)

    {GPIOB, GPIO_PIN_4}, // ENC0_A
    {GPIOB, GPIO_PIN_5}, // ENC0_B
    {GPIOA, GPIO_PIN_15}, // ENC0_Z
    {GPIOB, GPIO_PIN_6}, // ENC1_A
    {GPIOB, GPIO_PIN_7}, // ENC1_B
    {GPIOB, GPIO_PIN_3}, // ENC1_Z
    {GPIOB, GPIO_PIN_8}, // CAN_R
    {GPIOB, GPIO_PIN_9}, // CAN_D
};
#elif (HW_VERSION_MINOR == 3) || (HW_VERSION_MINOR == 4)
Stm32Gpio gpios[] = {
    {nullptr, 0}, // dummy GPIO0 so that PCB labels and software numbers match

    {GPIOA, GPIO_PIN_0}, // GPIO1
    {GPIOA, GPIO_PIN_1}, // GPIO2
    {GPIOA, GPIO_PIN_2}, // GPIO3
    {GPIOA, GPIO_PIN_3}, // GPIO4
    {GPIOB, GPIO_PIN_2}, // GPIO5
    {nullptr, 0}, // GPIO6 (doesn't exist on this board)
    {nullptr, 0}, // GPIO7 (doesn't exist on this board)
    {nullptr, 0}, // GPIO8 (doesn't exist on this board)

    {GPIOB, GPIO_PIN_4}, // ENC0_A
    {GPIOB, GPIO_PIN_5}, // ENC0_B
    {GPIOA, GPIO_PIN_15}, // ENC0_Z
    {GPIOB, GPIO_PIN_6}, // ENC1_A
    {GPIOB, GPIO_PIN_7}, // ENC1_B
    {GPIOB, GPIO_PIN_3}, // ENC1_Z
    {GPIOB, GPIO_PIN_8}, // CAN_R
    {GPIOB, GPIO_PIN_9}, // CAN_D
};
#elif (HW_VERSION_MINOR == 5) || (HW_VERSION_MINOR == 6)
Stm32Gpio gpios[GPIO_COUNT] = {
    {nullptr, 0}, // dummy GPIO0 so that PCB labels and software numbers match

    {GPIOA, GPIO_PIN_0}, // GPIO1
    {GPIOA, GPIO_PIN_1}, // GPIO2
    {GPIOA, GPIO_PIN_2}, // GPIO3
    {GPIOA, GPIO_PIN_3}, // GPIO4
    {GPIOC, GPIO_PIN_4}, // GPIO5
    {GPIOB, GPIO_PIN_2}, // GPIO6
    {GPIOA, GPIO_PIN_15}, // GPIO7
    {GPIOB, GPIO_PIN_3}, // GPIO8
    
    {GPIOB, GPIO_PIN_4}, // ENC0_A
    {GPIOB, GPIO_PIN_5}, // ENC0_B
    {GPIOC, GPIO_PIN_9}, // ENC0_Z
    {GPIOB, GPIO_PIN_6}, // ENC1_A
    {GPIOB, GPIO_PIN_7}, // ENC1_B
    {GPIOC, GPIO_PIN_15}, // ENC1_Z
    {GPIOB, GPIO_PIN_8}, // CAN_R
    {GPIOB, GPIO_PIN_9}, // CAN_D
};
#else
#error "unknown GPIOs"
#endif

std::array<GpioFunction, 3> alternate_functions[GPIO_COUNT] = {
    /* GPIO0 (inexistent): */ {{}},

#if HW_VERSION_MINOR >= 3
    /* GPIO1: */ {{{ODrive::GPIO_MODE_UART_A, GPIO_AF8_UART4}, {ODrive::GPIO_MODE_PWM, GPIO_AF2_TIM5}}},
    /* GPIO2: */ {{{ODrive::GPIO_MODE_UART_A, GPIO_AF8_UART4}, {ODrive::GPIO_MODE_PWM, GPIO_AF2_TIM5}}},
    /* GPIO3: */ {{{ODrive::GPIO_MODE_UART_B, GPIO_AF7_USART2}, {ODrive::GPIO_MODE_PWM, GPIO_AF2_TIM5}}},
#else
    /* GPIO1: */ {{}},
    /* GPIO2: */ {{}},
    /* GPIO3: */ {{}},
#endif

    /* GPIO4: */ {{{ODrive::GPIO_MODE_UART_B, GPIO_AF7_USART2}, {ODrive::GPIO_MODE_PWM, GPIO_AF2_TIM5}}},
    /* GPIO5: */ {{}},
    /* GPIO6: */ {{}},
    /* GPIO7: */ {{}},
    /* GPIO8: */ {{}},
    /* ENC0_A: */ {{{ODrive::GPIO_MODE_ENC0, GPIO_AF2_TIM3}}},
    /* ENC0_B: */ {{{ODrive::GPIO_MODE_ENC0, GPIO_AF2_TIM3}}},
    /* ENC0_Z: */ {{}},
    /* ENC1_A: */ {{{ODrive::GPIO_MODE_I2C_A, GPIO_AF4_I2C1}, {ODrive::GPIO_MODE_ENC1, GPIO_AF2_TIM4}}},
    /* ENC1_B: */ {{{ODrive::GPIO_MODE_I2C_A, GPIO_AF4_I2C1}, {ODrive::GPIO_MODE_ENC1, GPIO_AF2_TIM4}}},
    /* ENC1_Z: */ {{}},
    /* CAN_R: */ {{{ODrive::GPIO_MODE_CAN_A, GPIO_AF9_CAN1}, {ODrive::GPIO_MODE_I2C_A, GPIO_AF4_I2C1}}},
    /* CAN_D: */ {{{ODrive::GPIO_MODE_CAN_A, GPIO_AF9_CAN1}, {ODrive::GPIO_MODE_I2C_A, GPIO_AF4_I2C1}}},
};


/* DMA Streams ---------------------------------------------------------------*/

Stm32DmaStreamRef spi_rx_dma = {DMA1_Stream0};
Stm32DmaStreamRef spi_tx_dma = {DMA1_Stream7};
Stm32DmaStreamRef uart_a_rx_dma = {DMA1_Stream2};
Stm32DmaStreamRef uart_a_tx_dma = {DMA1_Stream4};
Stm32DmaStreamRef uart_b_rx_dma = {DMA1_Stream5};
Stm32DmaStreamRef uart_b_tx_dma = {DMA1_Stream6};


/* Communication interfaces --------------------------------------------------*/

Stm32Spi spi = {SPI3, spi_rx_dma, spi_tx_dma};
Stm32SpiArbiter spi_arbiter{&spi};

Stm32Usart uart_a = {UART4, uart_a_rx_dma, uart_a_tx_dma};
Stm32Usart uart_b = {USART2, uart_b_rx_dma, uart_b_tx_dma};
Stm32Usart uart_c = {nullptr, {0}, {0}};

Stm32Can can_a = {CAN1};

std::array<CanBusBase*, 1> can_busses = {&can_a};

#if HW_VERSION_MINOR <= 2
PwmInput pwm0_input{&htim5, {0, 0, 0, 4}}; // 0 means not in use
#else
PwmInput pwm0_input{&htim5, {1, 2, 3, 4}};
#endif

extern PCD_HandleTypeDef hpcd_USB_OTG_FS; // defined in usbd_conf.c
USBD_HandleTypeDef usb_dev_handle;


/* Onboard devices -----------------------------------------------------------*/

Drv8301 m0_gate_driver{
    &spi_arbiter,
    drv0_ncs_gpio, // nCS
    {}, // EN pin (shared between both motors, therefore we actuate it outside of the drv8301 driver)
    drv_nfault_gpio // nFAULT pin (shared between both motors)
};

Drv8301 m1_gate_driver{
    &spi_arbiter,
    drv1_ncs_gpio, // nCS
    {}, // EN pin (shared between both motors, therefore we actuate it outside of the drv8301 driver)
    drv_nfault_gpio // nFAULT pin (shared between both motors)
};

const float fet_thermistor_poly_coeffs[] =
    {363.93910201f, -462.15369634f, 307.55129571f, -27.72569531f};
const size_t fet_thermistor_num_coeffs = sizeof(fet_thermistor_poly_coeffs)/sizeof(fet_thermistor_poly_coeffs[1]);

OnboardThermistorCurrentLimiter fet_thermistors[AXIS_COUNT] = {
    {
        15, // adc_channel
        &fet_thermistor_poly_coeffs[0], // coefficients
        fet_thermistor_num_coeffs // num_coeffs
    }, {
#if HW_VERSION_MAJOR == 3 && HW_VERSION_MINOR >= 3
        4, // adc_channel
#else
        1, // adc_channel
#endif
        &fet_thermistor_poly_coeffs[0], // coefficients
        fet_thermistor_num_coeffs // num_coeffs
    }
};

OffboardThermistorCurrentLimiter motor_thermistors[AXIS_COUNT];

Motor motors[AXIS_COUNT] = {
    {
        &htim1, // timer
        0b110, // current_sensor_mask
        1.0f / SHUNT_RESISTANCE, // shunt_conductance [S]
        m0_gate_driver, // gate_driver
        m0_gate_driver, // opamp
        fet_thermistors[0],
        motor_thermistors[0]
    },
    {
        &htim8, // timer
        0b110, // current_sensor_mask
        1.0f / SHUNT_RESISTANCE, // shunt_conductance [S]
        m1_gate_driver, // gate_driver
        m1_gate_driver, // opamp
        fet_thermistors[1],
        motor_thermistors[1]
    }
};

Encoder encoders[AXIS_COUNT] = {
    {
        &htim3, // timer
        gpios[11], // index_gpio
        gpios[9], // hallA_gpio
        gpios[10], // hallB_gpio
        gpios[11], // hallC_gpio
        &spi_arbiter // spi_arbiter
    },
    {
        &htim4, // timer
        gpios[14], // index_gpio
        gpios[12], // hallA_gpio
        gpios[13], // hallB_gpio
        gpios[14], // hallC_gpio
        &spi_arbiter // spi_arbiter
    }
};

// TODO: this has no hardware dependency and should be allocated depending on config
Endstop endstops[2 * AXIS_COUNT];
MechanicalBrake mechanical_brakes[AXIS_COUNT];

SensorlessEstimator sensorless_estimators[AXIS_COUNT];
Controller controllers[AXIS_COUNT];
TrapezoidalTrajectory trap[AXIS_COUNT];

std::array<Axis, AXIS_COUNT> axes{{
    {
        0, // axis_num
        1, // step_gpio_pin
        2, // dir_gpio_pin
        (osPriority)(osPriorityHigh + (osPriority)1), // thread_priority
        encoders[0], // encoder
        sensorless_estimators[0], // sensorless_estimator
        controllers[0], // controller
        motors[0], // motor
        trap[0], // trap
        endstops[0], endstops[1], // min_endstop, max_endstop
        mechanical_brakes[0], // mechanical brake
    },
    {
        1, // axis_num
#if HW_VERSION_MAJOR == 3 && HW_VERSION_MINOR >= 5
        7, // step_gpio_pin
        8, // dir_gpio_pin
#else
        3, // step_gpio_pin
        4, // dir_gpio_pin
#endif
        osPriorityHigh, // thread_priority
        encoders[1], // encoder
        sensorless_estimators[1], // sensorless_estimator
        controllers[1], // controller
        motors[1], // motor
        trap[1], // trap
        endstops[2], endstops[3], // min_endstop, max_endstop
        mechanical_brakes[1], // mechanical brake
    },
}};

Stm32BasicPwmOutput<TIM_APB1_PERIOD_CLOCKS, TIM_APB1_DEADTIME_CLOCKS> brake_resistor_output_impl{TIM2->CCR3, TIM2->CCR4};
PwmOutputGroup<1>& brake_resistor_output = brake_resistor_output_impl;

/* Misc Variables ------------------------------------------------------------*/

volatile uint32_t& board_control_loop_counter = TIM13->CNT;
uint32_t board_control_loop_counter_period = CONTROL_TIMER_PERIOD_TICKS / 2; // TIM13 is on a clock that's only half as fast as TIM1


static bool check_board_version(const uint8_t* otp_ptr) {
    return (otp_ptr[3] == HW_VERSION_MAJOR) &&
           (otp_ptr[4] == HW_VERSION_MINOR) &&
           (otp_ptr[5] == HW_VERSION_VOLTAGE);
}

void system_init() {
    // Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // Configure the system clock
    SystemClock_Config();

    // If the OTP is pristine, use the fake-otp in RAM instead
    const uint8_t* otp_ptr = (const uint8_t*)FLASH_OTP_BASE;
    if (*otp_ptr == 0xff) {
        otp_ptr = fake_otp;
    }

    // Ensure that the board version for which this firmware is compiled matches
    // the board we're running on.
    if (!check_board_version(otp_ptr)) {
        for (;;);
    }
}

bool board_init() {

    // Init DMA interrupts

    nvic.enable_with_prio(spi_rx_dma.get_irqn(), 4);
    nvic.enable_with_prio(spi_tx_dma.get_irqn(), 3);

    // Init internal GPIOs (external GPIOs are initialized in main.cpp
    // depending on user config)

    vbus_s_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    aux_fet_temp_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    m0_fet_temp_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    m1_fet_temp_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    m0_sob_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    m0_soc_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    m1_sob_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);
    m1_soc_gpio.config(GPIO_MODE_ANALOG, GPIO_NOPULL);

    drv0_ncs_gpio.write(true);
    drv0_ncs_gpio.config(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    drv1_ncs_gpio.write(true);
    drv1_ncs_gpio.config(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);
    drv_nfault_gpio.config(GPIO_MODE_INPUT, GPIO_PULLUP);
    drv_en_gate_gpio.write(false);
    drv_en_gate_gpio.config(GPIO_MODE_OUTPUT_PP, GPIO_NOPULL);

    spi_miso_gpio.config(GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);
    spi_mosi_gpio.config(GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);
    spi_clk_gpio.config(GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_SPEED_FREQ_VERY_HIGH, GPIO_AF6_SPI3);

    // Call CubeMX-generated peripheral initialization functions

    MX_ADC1_Init();
    MX_ADC2_Init();
    MX_TIM1_Init();
    MX_TIM8_Init();
    MX_TIM3_Init();
    MX_TIM4_Init();
    MX_ADC3_Init();
    MX_TIM2_Init();
    MX_TIM5_Init();
    MX_TIM13_Init();

    // Init USB peripheral control driver
    hpcd_USB_OTG_FS.pData = &usb_dev_handle;
    usb_dev_handle.pData = &hpcd_USB_OTG_FS;
    
    hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
    hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
    hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.ep0_mps = DEP0CTL_MPS_64;
    hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.vbus_sensing_enable = DISABLE;
    hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
    if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK) {
        return false;
    }

    HAL_PCDEx_SetRxFiFo(&hpcd_USB_OTG_FS, 0x80);
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 0, 0x40);
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 1, 0x40); // CDC IN endpoint
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_OTG_FS, 3, 0x40); // ODrive IN endpoint

    // External interrupt lines are individually enabled in stm32_gpio.cpp
    nvic.enable_with_prio(EXTI0_IRQn, 1);
    nvic.enable_with_prio(EXTI1_IRQn, 1);
    nvic.enable_with_prio(EXTI2_IRQn, 1);
    nvic.enable_with_prio(EXTI3_IRQn, 1);
    nvic.enable_with_prio(EXTI4_IRQn, 1);
    nvic.enable_with_prio(EXTI9_5_IRQn, 1);
    nvic.enable_with_prio(EXTI15_10_IRQn, 1);

    nvic.enable_with_prio(ControlLoop_IRQn, 5);
    nvic.enable_with_prio(TIM8_UP_TIM13_IRQn, 0);

    spi.init();

    if (uart_a && odrv.config_.enable_uart_a) {
        nvic.enable_with_prio(uart_a_rx_dma.get_irqn(), 10);
        nvic.enable_with_prio(uart_a_tx_dma.get_irqn(), 10);
        nvic.enable_with_prio(uart_a.get_irqn(), 10);
        if (!uart_a.init(odrv.config_.uart_a_baudrate)) {
            return false; // TODO: continue startup in degraded state
        }
    } else if (odrv.config_.enable_uart_a) {
        odrv.misconfigured_ = true;
    }

    if (uart_b && odrv.config_.enable_uart_b) {
        nvic.enable_with_prio(uart_b_rx_dma.get_irqn(), 10);
        nvic.enable_with_prio(uart_b_tx_dma.get_irqn(), 10);
        nvic.enable_with_prio(uart_b.get_irqn(), 10);
        if (!uart_b.init(odrv.config_.uart_b_baudrate)) {
            return false; // TODO: continue startup in degraded state
        }
    } else if (odrv.config_.enable_uart_b) {
        odrv.misconfigured_ = true;
    }

    if (odrv.config_.enable_uart_c) {
        odrv.misconfigured_ = true;
    }

    if (odrv.config_.enable_i2c_a) {
        
        /* I2C support currently not maintained
        // Set up the direction GPIO as input
        get_gpio(3).config(GPIO_MODE_INPUT, GPIO_PULLUP);
        get_gpio(4).config(GPIO_MODE_INPUT, GPIO_PULLUP);
        get_gpio(5).config(GPIO_MODE_INPUT, GPIO_PULLUP);

        osDelay(1); // This has no effect but was here before.
        i2c_stats_.addr = (0xD << 3);
        i2c_stats_.addr |= get_gpio(3).read() ? 0x1 : 0;
        i2c_stats_.addr |= get_gpio(4).read() ? 0x2 : 0;
        i2c_stats_.addr |= get_gpio(5).read() ? 0x4 : 0;
        MX_I2C1_Init(i2c_stats_.addr);*/

        odrv.misconfigured_ = true;
    }

    if (odrv.config_.enable_can_a) {
        // The CAN initialization will (and must) init its own GPIOs before the
        // GPIO modes are initialized. Therefore we ensure that the later GPIO
        // mode initialization won't override the CAN mode.
        if (odrv.config_.gpio_modes[15] != ODriveIntf::GPIO_MODE_CAN_A || odrv.config_.gpio_modes[16] != ODriveIntf::GPIO_MODE_CAN_A) {
            odrv.misconfigured_ = true;
        } else {
        
            nvic.enable_with_prio(CAN1_TX_IRQn, 9);
            nvic.enable_with_prio(CAN1_RX0_IRQn, 9);
            nvic.enable_with_prio(CAN1_RX1_IRQn, 9);
            nvic.enable_with_prio(CAN1_SCE_IRQn, 9);

            if (!can_a.init({
                .Prescaler = 8,
                .Mode = CAN_MODE_NORMAL,
                .SyncJumpWidth = CAN_SJW_4TQ,
                .TimeSeg1 = CAN_BS1_16TQ,
                .TimeSeg2 = CAN_BS2_4TQ,
                .TimeTriggeredMode = DISABLE,
                .AutoBusOff = ENABLE,
                .AutoWakeUp = ENABLE,
                .AutoRetransmission = ENABLE,
                .ReceiveFifoLocked = DISABLE,
                .TransmitFifoPriority = DISABLE,
            }, 2000000UL)) {
                return false; // TODO: continue in degraded mode
            }

        }
    }

    // Ensure that debug halting of the core doesn't leave the motor PWM running
    __HAL_DBGMCU_FREEZE_TIM1();
    __HAL_DBGMCU_FREEZE_TIM8();
    __HAL_DBGMCU_FREEZE_TIM13();

    // Start brake resistor PWM in floating output configuration
    htim2.Instance->CCR3 = 0;
    htim2.Instance->CCR4 = TIM_APB1_PERIOD_CLOCKS + 1;
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

    // Reset both DRV chips. The enable pin also controls the SPI interface, not
    // only the driver stages.
    drv_en_gate_gpio.write(false);
    delay_us(40); // mimumum pull-down time for full reset: 20us
    drv_en_gate_gpio.write(true);
    delay_us(20000); // mimumum pull-down time for full reset: 20us

    return true;
}

void start_timers() {
    CRITICAL_SECTION() {
        // Temporarily disable ADC triggers so they don't trigger as a side
        // effect of starting the timers.
        hadc1.Instance->CR2 &= ~(ADC_CR2_JEXTEN);
        hadc2.Instance->CR2 &= ~(ADC_CR2_EXTEN | ADC_CR2_JEXTEN);
        hadc3.Instance->CR2 &= ~(ADC_CR2_EXTEN | ADC_CR2_JEXTEN);

        /*
        * Synchronize TIM1, TIM8 and TIM13 such that:
        *  1. The triangle waveform of TIM1 leads the triangle waveform of TIM8 by a
        *     90° phase shift.
        *  2. Each TIM13 reload coincides with a TIM1 lower update event.
        */
        Stm32Timer::start_synchronously<3>(
            {&htim1, &htim8, &htim13},
            {TIM1_INIT_COUNT, 0, board_control_loop_counter}
        );

        hadc1.Instance->CR2 |= (ADC_EXTERNALTRIGINJECCONVEDGE_RISING);
        hadc2.Instance->CR2 |= (ADC_EXTERNALTRIGCONVEDGE_RISING | ADC_EXTERNALTRIGINJECCONVEDGE_RISING);
        hadc3.Instance->CR2 |= (ADC_EXTERNALTRIGCONVEDGE_RISING | ADC_EXTERNALTRIGINJECCONVEDGE_RISING);

        __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_JEOC);
        __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_JEOC);
        __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_JEOC);
        __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_EOC);
        __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_EOC);
        __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_EOC);
        __HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_OVR);
        __HAL_ADC_CLEAR_FLAG(&hadc2, ADC_FLAG_OVR);
        __HAL_ADC_CLEAR_FLAG(&hadc3, ADC_FLAG_OVR);
        
        __HAL_TIM_CLEAR_IT(&htim8, TIM_IT_UPDATE);
        __HAL_TIM_ENABLE_IT(&htim8, TIM_IT_UPDATE);
    }
}

static bool fetch_and_reset_adcs(
        std::optional<Iph_ABC_t>* current0,
        std::optional<Iph_ABC_t>* current1) {
    bool all_adcs_done = (ADC1->SR & ADC_SR_JEOC) == ADC_SR_JEOC
        && (ADC2->SR & (ADC_SR_EOC | ADC_SR_JEOC)) == (ADC_SR_EOC | ADC_SR_JEOC)
        && (ADC3->SR & (ADC_SR_EOC | ADC_SR_JEOC)) == (ADC_SR_EOC | ADC_SR_JEOC);
    if (!all_adcs_done) {
        return false;
    }

    vbus_sense_adc_cb(ADC1->JDR1);

    if (m0_gate_driver.is_ready()) {
        std::optional<float> phB = motors[0].phase_current_from_adcval(ADC2->JDR1);
        std::optional<float> phC = motors[0].phase_current_from_adcval(ADC3->JDR1);
        if (phB.has_value() && phC.has_value()) {
            *current0 = {-*phB - *phC, *phB, *phC};
        }
    }

    if (m1_gate_driver.is_ready()) {
        std::optional<float> phB = motors[1].phase_current_from_adcval(ADC2->DR);
        std::optional<float> phC = motors[1].phase_current_from_adcval(ADC3->DR);
        if (phB.has_value() && phC.has_value()) {
            *current1 = {-*phB - *phC, *phB, *phC};
        }
    }
    
    ADC1->SR = ~(ADC_SR_JEOC);
    ADC2->SR = ~(ADC_SR_EOC | ADC_SR_JEOC | ADC_SR_OVR);
    ADC3->SR = ~(ADC_SR_EOC | ADC_SR_JEOC | ADC_SR_OVR);

    return true;
}

extern "C" {

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    HAL_SPI_TxRxCpltCallback(hspi);
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    HAL_SPI_TxRxCpltCallback(hspi);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi == &spi.hspi_) {
        spi_arbiter.on_complete();
    }
}

void TIM5_IRQHandler(void) {
    COUNT_IRQ(TIM5_IRQn);
    pwm0_input.on_capture();
}

volatile uint32_t timestamp_ = 0;
volatile bool counting_down_ = false;

void TIM8_UP_TIM13_IRQHandler(void) {
    COUNT_IRQ(TIM8_UP_TIM13_IRQn);
    
    // Entry into this function happens at 21-23 clock cycles after the timer
    // update event.
    __HAL_TIM_CLEAR_IT(&htim8, TIM_IT_UPDATE);

    // If the corresponding timer is counting up, we just sampled in SVM vector 0, i.e. real current
    // If we are counting down, we just sampled in SVM vector 7, with zero current
    bool counting_down = TIM8->CR1 & TIM_CR1_DIR;

    bool timer_update_missed = (counting_down_ == counting_down);
    if (timer_update_missed) {
        motors[0].disarm_with_error(Motor::ERROR_TIMER_UPDATE_MISSED);
        motors[1].disarm_with_error(Motor::ERROR_TIMER_UPDATE_MISSED);
        return;
    }
    counting_down_ = counting_down;

    timestamp_ += TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1);

    if (!counting_down) {
        TaskTimer::enabled = odrv.task_timers_armed_;
        // Run sampling handlers and kick off control tasks when TIM8 is
        // counting up.
        odrv.sampling_cb();
        NVIC->STIR = ControlLoop_IRQn;
    } else {
        // Tentatively reset all PWM outputs to 50% duty cycles. If the control
        // loop handler finishes in time then these values will be overridden
        // before they go into effect.
        TIM1->CCR1 =
        TIM1->CCR2 =
        TIM1->CCR3 =
        TIM8->CCR1 =
        TIM8->CCR2 =
        TIM8->CCR3 =
            TIM_1_8_PERIOD_CLOCKS / 2;
    }
}

void ControlLoop_IRQHandler(void) {
    COUNT_IRQ(ControlLoop_IRQn);
    uint32_t timestamp = timestamp_;

    // Ensure that all the ADCs are done
    std::optional<Iph_ABC_t> current0;
    std::optional<Iph_ABC_t> current1;

    if (!fetch_and_reset_adcs(&current0, &current1)) {
        motors[0].disarm_with_error(Motor::ERROR_BAD_TIMING);
        motors[1].disarm_with_error(Motor::ERROR_BAD_TIMING);
    }

    // If the motor FETs are not switching then we can't measure the current
    // because for this we need the low side FET to conduct.
    // So for now we guess the current to be 0 (this is not correct shortly after
    // disarming and when the motor spins fast in idle). Passing an invalid
    // current reading would create problems with starting FOC.
    if (!(TIM1->BDTR & TIM_BDTR_MOE_Msk)) {
        current0 = {0.0f, 0.0f};
    }
    if (!(TIM8->BDTR & TIM_BDTR_MOE_Msk)) {
        current1 = {0.0f, 0.0f};
    }

    motors[0].current_meas_cb(timestamp - TIM1_INIT_COUNT, current0);
    motors[1].current_meas_cb(timestamp, current1);

    odrv.control_loop_cb(timestamp);

    // By this time the ADCs for both M0 and M1 should have fired again. But
    // let's wait for them just to be sure.
    MEASURE_TIME(odrv.task_times_.dc_calib_wait) {
        while (!(ADC2->SR & ADC_SR_EOC));
    }

    if (!fetch_and_reset_adcs(&current0, &current1)) {
        motors[0].disarm_with_error(Motor::ERROR_BAD_TIMING);
        motors[1].disarm_with_error(Motor::ERROR_BAD_TIMING);
    }

    motors[0].dc_calib_cb(timestamp + TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1) - TIM1_INIT_COUNT, current0);
    motors[1].dc_calib_cb(timestamp + TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1), current1);

    motors[0].pwm_update_cb(timestamp + 3 * TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1) - TIM1_INIT_COUNT);
    motors[1].pwm_update_cb(timestamp + 3 * TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1));

    // TODO: move to main control loop. For now we put it here because the motor
    // PWM update refreshes the power estimate and the brake res update must
    // happen after that.
    odrv.brake_resistor_.update();

    // The brake resistor PWM is not latched on TIM1 update events but will take
    // effect immediately. This means that the timestamp given here is a bit too
    // late.
    brake_resistor_output_impl.update(timestamp + 3 * TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1) - TIM1_INIT_COUNT);

    // If we did everything right, the TIM8 update handler should have been
    // called exactly once between the start of this function and now.

    if (timestamp_ != timestamp + TIM_1_8_PERIOD_CLOCKS * (TIM_1_8_RCR + 1)) {
        motors[0].disarm_with_error(Motor::ERROR_CONTROL_DEADLINE_MISSED);
        motors[1].disarm_with_error(Motor::ERROR_CONTROL_DEADLINE_MISSED);
    }

    odrv.task_timers_armed_ = odrv.task_timers_armed_ && !TaskTimer::enabled;
    TaskTimer::enabled = false;
}

/* I2C support currently not maintained
void I2C1_EV_IRQHandler(void) {
    COUNT_IRQ(I2C1_EV_IRQn);
    HAL_I2C_EV_IRQHandler(&hi2c1);
}

void I2C1_ER_IRQHandler(void) {
    COUNT_IRQ(I2C1_ER_IRQn);
    HAL_I2C_ER_IRQHandler(&hi2c1);
}*/

void DMA1_Stream2_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream2_IRQn);
    HAL_DMA_IRQHandler(&uart_a.hdma_rx_);
}

void DMA1_Stream4_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream4_IRQn);
    HAL_DMA_IRQHandler(&uart_a.hdma_tx_);
}

void DMA1_Stream5_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream5_IRQn);
    HAL_DMA_IRQHandler(&uart_b.hdma_rx_);
}

void DMA1_Stream6_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream6_IRQn);
    HAL_DMA_IRQHandler(&uart_b.hdma_tx_);
}

void UART4_IRQHandler(void) {
    COUNT_IRQ(UART4_IRQn);
    HAL_UART_IRQHandler(uart_a);
}

void USART2_IRQHandler(void) {
    COUNT_IRQ(USART2_IRQn);
    HAL_UART_IRQHandler(uart_b);
}

void DMA1_Stream0_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream0_IRQn);
    HAL_DMA_IRQHandler(&spi.hdma_rx_);
}

void DMA1_Stream7_IRQHandler(void) {
    COUNT_IRQ(DMA1_Stream7_IRQn);
    HAL_DMA_IRQHandler(&spi.hdma_tx_);
}

void SPI3_IRQHandler(void) {
    COUNT_IRQ(SPI3_IRQn);
    HAL_SPI_IRQHandler(spi);
}

void CAN1_TX_IRQHandler(void) {
    COUNT_IRQ(CAN1_TX_IRQn);
    HAL_CAN_IRQHandler(&can_a.handle_);
}

void CAN1_RX0_IRQHandler(void) {
    COUNT_IRQ(CAN1_RX0_IRQn);
    HAL_CAN_IRQHandler(&can_a.handle_);
}

void CAN1_RX1_IRQHandler(void) {
    COUNT_IRQ(CAN1_RX1_IRQn);
    HAL_CAN_IRQHandler(&can_a.handle_);
}

void CAN1_SCE_IRQHandler(void) {
    COUNT_IRQ(CAN1_SCE_IRQn);
    HAL_CAN_IRQHandler(&can_a.handle_);
}

void OTG_FS_IRQHandler(void) {
    COUNT_IRQ(OTG_FS_IRQn);
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}

}
