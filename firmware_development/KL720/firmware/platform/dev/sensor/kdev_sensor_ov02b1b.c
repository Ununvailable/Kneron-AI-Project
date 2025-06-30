#if (IMGSRC_0_SENSORID == SENSOR_TYPE_OV02B1B_R) || \
    (IMGSRC_1_SENSORID == SENSOR_TYPE_OV02B1B_R) || \
    (IMGSRC_0_SENSORID == SENSOR_TYPE_OV02B1B_L) || \
    (IMGSRC_1_SENSORID == SENSOR_TYPE_OV02B1B_L)
#include <stdlib.h>
#include "project.h"
#include "kdev_sensor.h"
#include "kdrv_i2c.h"
#include "kdrv_gpio.h"
#include "kdrv_clock.h"
#include "base.h"
#define TFT43_WIDTH             480
#define TFT43_HEIGHT            272

#define VGA_LANDSCAPE_WIDTH     640
#define VGA_LANDSCAPE_HEIGHT    480
#define VGA_PORTRAIT_WIDTH      480
#define VGA_PORTRAIT_HEIGHT     640

#define HD_WIDTH                1280
#define HD_HEIGHT               720

#define FHD_WIDTH               1920
#define FHD_HEIGHT              1080

#define HMX_RICA_WIDTH          864
#define HMX_RICA_HEIGHT         491

#define QVGA_WIDTH              1280
#define QVGA_HEIGHT             960

#define UGA_WIDTH               1600
#define UGA_HEIGHT              1200

#define SVGA_WIDTH              800
#define SVGA_HEIGHT             600

#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))

#define RES_800X600 1

#define OB02b1B_DBG
#ifdef OB02b1B_DBG
#include "kmdw_console.h"
#define sensor_msg(fmt, ...) kmdw_printf("\n[%s] " fmt, __func__, ##__VA_ARGS__)
#else
#define sensor_msg(fmt, ...)
#endif

#define CROPPING_OFFSET_X   (0)
#define CROPPING_OFFSET_Y   (0)

#define ROI_OFFSET_X        ((CROPPING_OFFSET_X + 4) >> 3)
#define ROI_OFFSET_Y        ((CROPPING_OFFSET_Y + 4) >> 3)

#define DEV_LEFT        0
#define DEV_RIGHT       1
#define DEV_NUM         2

static uint8_t led_switch[2] = {0};

static const struct sensor_win_size ov02b1b_supported_win_sizes[] = {
    { .width = SVGA_WIDTH,           .height = SVGA_HEIGHT,           },
};

static struct sensor_device ov02b1b_dev[DEV_NUM] = {
   
    {
        .addr = 0x3d,
        .i2c_port =  IMGSRC_1_PORT_ID,
    },
    {
        .addr = 0x3c,
        .i2c_port =  IMGSRC_0_PORT_ID,
    }
};

struct sensor_init_seq ov02b1b_init_regs[] = {  
#ifdef RES_800X600
{0xfd,0x00},               
{0xfd,0x00},  
{0x24,0x02},   //pll_mc
{0x25,0x06},   //pll_nc,dpll clk 72M
{0x29,0x03},
{0x2a,0xb4},   //mpll_nc, mpll clk 330M
{0x1e,0x17},   //vlow 0.53v
{0x33,0x07},   //ipx 2.84u
{0x35,0x07},   //pcp off
{0x4a,0x0c},   //ncp -1.4v
{0x3a,0x05},   //icomp1 4.25u
{0x3b,0x02},   //icomp2 1.18u
{0x3e,0x00},
{0x46,0x01},
{0xfd,0x01},

{0x14,0x07},  
{0x15,0x08},   //VTS dummy

{0x0e,0x03},  
{0x0f,0xe8},   //exp
{0x18,0x00},   //un fixed-fps
{0x22,0x40},   //analog gain
{0x23,0x02},   //adc_range 0.595v
{0x17,0x2c},   //pd reset row address time
{0x19,0x20},   //dac_d0 1024
{0x1b,0x06},   //rst_num1 96
{0x1c,0x04},   //rst_num2 64
{0x20,0x03},
{0x30,0x01},   //p0
{0x33,0x01},   //p3
{0x31,0x0a},   //p1
{0x32,0x09},   //p2
{0x38,0x01},
{0x39,0x01},   //p9
{0x3a,0x01},   //p10
{0x3b,0x01},
{0x4f,0x04},   //p24
{0x4e,0x05},   //p23
{0x50,0x01},   //p25   
{0x35,0x0c},   //p5
{0x45,0x2a},   //sc1,p20_1
{0x46,0x2a},   //p20_2
{0x47,0x2a},   //p20_3
{0x48,0x2a},   //p20_4
{0x4a,0x2c},   //sc2,p22_1
{0x4b,0x2c},   //p22_2
{0x4c,0x2c},   //p22_3
{0x4d,0x2c},   //p22_4
{0x56,0x3a},   //p31, 1st d0
{0x57,0x0a},   //p32, 1st d1
{0x58,0x24},   //col_en1
{0x59,0x20},   //p34 2nd d0
{0x5a,0x0a},   //p34 2nd d1
{0x5b,0xff},   //col_en2
{0x37,0x0a},   //p7, tx
{0x42,0x0e},   //p17, psw 
{0x68,0x90},
{0x69,0xcd},   //blk en, no sig_clamp
{0x7c,0x08},
{0x7d,0x08},
{0x7e,0x08},
{0x7f,0x08},   //vbl1_4
{0x83,0x14},
{0x84,0x14},
{0x86,0x14},
{0x87,0x07},   //vbl2_4
{0x88,0x0f},
{0x94,0x02},   //evsync del frame 
{0x98,0xd1},   //del bad frame
{0xfe,0x02},
{0xfd,0x03},   //RegPage
{0x97,0x6c},
{0x98,0x60},
{0x99,0x60},
{0x9a,0x6c},
{0xae,0x0d},   //bit0=1,high 8bit
{0x88,0x49},   //BLC_ABL
{0x89,0x7c},   //bit6=1 trigger en
{0xb4,0x05},   //mean trigger 5
{0xbd,0x0d},   //blc_rpc_coe
{0x8c,0x40},   //BLC_BLUE_SUBOFFSET_8lsb
{0x8e,0x40},   //BLC_RED_SUBOFFSET_8lsb
{0x90,0x40},   //BLC_GR_SUBOFFSET_8lsb
{0x92,0x40},   // BLC_GB_SUBOFFSET_8lsb
{0x9b,0x49},   //digtal gain
{0xac,0x40},   //blc random noise rpc_th 4x
{0xfd,0x00},
{0x5a,0x15},
{0x74,0x01},   // PD_MIPIturn on mipi phy 

{0xfd,0x00},  //binning 800x600
{0x28,0x03},
{0x4f,0x03},  //mipi size
{0x50,0x20},
{0x51,0x02},
{0x52,0x58},
{0xfd,0x01},
{0x12,(0x00 | (0 << 1) | (0))},
{0x03,0x70},  //h-start
{0x05,0x10},  //v-start
{0x07,0x20},  
{0x09,0xb0},
{0x6c,0x09},  //binning22 en
{0xfe,0x02}, 
{0xfb,0x01},

//// raw8 output
{0xfd,0x00},
{0x55,0x2a},
{0x27,0x01},
{0x6e,0x02},

{0xfd,0x03},
{0xc2,0x01},
{0xfd,0x01},  

#else  
{0xfd,0x00},  
{0x24,0x02},   //pll_mc
{0x25,0x06},   //pll_nc,dpll clk 72M
{0x29,0x01},
{0x2a,0xb4},   //mpll_nc, mpll clk 660M
{0x2b,0x00},
{0x1e,0x17},   //vlow 0.53v
{0x33,0x07},   //ipx 2.84u
{0x35,0x07},   
{0x4a,0x0c},   //ncp -1.4v
{0x3a,0x05},   //icomp1 4.25u
{0x3b,0x02},   //icomp2 1.18u
{0x3e,0x00},
{0x46,0x01},
{0x6d,0x03},
{0xfd,0x01},  
{0x0e,0x02},  
{0x0f,0x1a},   //exp
{0x18,0x00},   //un fixed-fps
{0x22,0xff},   //analog gain
{0x23,0x02},   //adc_range 0.595v
{0x17,0x2c},   //pd reset row address ti
{0x19,0x20},   //dac_d0 1024
{0x1b,0x06},   //rst_num1 96
{0x1c,0x04},   //rst_num2 64
{0x20,0x03},
{0x30,0x01},   //p0
{0x33,0x01},   //p3
{0x31,0x0a},   //p1
{0x32,0x09},   //p2
{0x38,0x01},
{0x39,0x01},   //p9
{0x3a,0x01},   //p10
{0x3b,0x01},
{0x4f,0x04},   //p24
{0x4e,0x05},   //p23
{0x50,0x01},   //p25   
{0x35,0x0c},   //p5
{0x45,0x2a},   //sc1,p20_1
{0x46,0x2a},   //p20_2
{0x47,0x2a},   //p20_3
{0x48,0x2a},   //p20_4
{0x4a,0x2c},   //sc2,p22_1
{0x4b,0x2c},   //p22_2
{0x4c,0x2c},   //p22_3
{0x4d,0x2c},   //p22_4
{0x56,0x3a},   //p31, 1st d0
{0x57,0x0a},   //p32, 1st d1
{0x58,0x24},   //col_en1
{0x59,0x20},   //p34 2nd d0
{0x5a,0x0a},   //p34 2nd d1
{0x5b,0xff},   //col_en2
{0x37,0x0a},   //p7, tx
{0x42,0x0e},   //p17, psw 
{0x68,0x90},
{0x69,0xcd},   //blk en, no sig_clamp
{0x6a,0x8f},
{0x7c,0x0a},
{0x7d,0x09},	//0a
{0x7e,0x09},	//0a
{0x7f,0x08},   
{0x83,0x14},
{0x84,0x14},
{0x86,0x14},
{0x87,0x07},   //vbl2_4
{0x88,0x0f},
{0x94,0x02},   //evsync del frame 
{0x98,0xd1},   //del bad frame
{0xfe,0x02},
{0xfd,0x03},   //RegPage
{0x97,0x78},
{0x98,0x78},
{0x99,0x78},
{0x9a,0x78},
{0xa1,0x40},
{0xb1,0x30},
{0xae,0x0d},   //bit0=1,high 8bit
{0x88,0x5b},   //BLC_ABL
{0x89,0x7c},   //bit6=1 trigger en
{0xb4,0x05},   //mean trigger 5
{0x8c,0x40},   //BLC_BLUE_SUBOFFSET_8lsb
{0x8e,0x40},   //BLC_RED_SUBOFFSET_8lsb
{0x90,0x40},   //BLC_GR_SUBOFFSET_8lsb
{0x92,0x40},   // BLC_GB_SUBOFFSET_8lsb
{0x9b,0x46},   //digtal gain
{0xac,0x40},   //blc random noise rpc_th
{0xfd,0x00},
{0x5a,0x15},
{0x74,0x01},   // PD_MIPIturn on mipi ph

{0xfd,0x00},  //crop to 1600x1200
{0x50,0x40},  //mipi hszie low 8bit
{0x52,0xb0},  //mipi vsize low 8bit
{0xfd,0x01},
{0x03,0x70},  //window hstart low 8bit
{0x05,0x10},  //window vstart low 8bit
{0x07,0x20},  //window hsize low 8bit
{0x09,0xb0},  //window vsize low 8bit


{0xfb,0x01},

// raw8 output
{0xfd,0x00},
{0x55,0x2a},
{0x27,0x01},gc1054
{0x6e,0x02},


//stream on
{0xfd,0x03},
{0xc2,0x01},  //MIPI_EN
{0xfd,0x01}, 
#endif
  {0x00, 0x00},
};

#if 1// ( CFG_LED_DRIVER_TYPE == LED_DRIVER_AW36515 )
typedef enum __io_led_mode
{
    LED_STANDBY_MODE = 0x80,
    LED_TORCH_MODE = 0x08,//0x88,
    LED_FLASH_MODE = 0x8C,
} io_led_mode;

static io_led_mode g_LedLightMode = LED_STANDBY_MODE;
#define I2C_ADAP_0      0
#define I2C_LED_DEVICE      KDRV_I2C_CTRL_2
#define LED_SLAVE_ID        (0x63)
#define IO_LED1      ( 1 << 0 )
#define IO_LED2      ( 1 << 1 )
extern void led_set_light_mode( io_led_mode nMode );
void led_set_light_mode( io_led_mode nMode )
{
    g_LedLightMode = nMode;
}

static void aw36515_init( io_led_mode nMode )
{
     static uint8_t nFirstInit = true;
     uint8_t nDeviceId;
     uint16_t data;
     if ( nFirstInit == true )
     {
         //check device id
         kdrv_i2c_read_register( I2C_LED_DEVICE, LED_SLAVE_ID, 0x00, 1, 1, (uint16_t*)&nDeviceId );
         sensor_msg("  36515 nDeviceId = 0x%x\n",  nDeviceId);
         if ( nDeviceId != 0x30 )
             return;

         //Set LED timing
         data = 0x1F;
         kdrv_i2c_write_register( I2C_LED_DEVICE, LED_SLAVE_ID, 0x08, 1, 1, &data );

         //Set enable register
         led_set_light_mode( nMode );
         kdrv_i2c_write_register( I2C_LED_DEVICE, LED_SLAVE_ID, 0x01, 1, 1, (uint16_t*)&g_LedLightMode );  //set mode and turn off LED
         nFirstInit = false;
     }
}
void nir_led_open(io_led_index led_index)
{
    uint8_t needOpenLed = 0;
    uint16_t led_status = 0;
    uint16_t data = 0;

    if ( (led_index == LED_ONLY_ONE || led_index == LED_EDGE) && (led_switch[LED_EDGE] == 0) ) 
    {
        data |= (g_LedLightMode | IO_LED1);
        led_switch[LED_EDGE] = 1;
        needOpenLed = 1;
    }
    else if ( (led_index == LED_CENTER) && (led_switch[LED_CENTER] == 0) )
    {
        data |= (g_LedLightMode | IO_LED2);
        led_switch[LED_CENTER] = 1;
        needOpenLed = 1;
    }
    else if ( (led_index == LED_BOTH) && (led_switch[LED_EDGE] == 0 || led_switch[LED_CENTER] == 0) )
    {
        data |= (g_LedLightMode | IO_LED1 | IO_LED2);
        led_switch[LED_EDGE] = 1;
        led_switch[LED_CENTER] = 1;
        needOpenLed = 1;
    }
    
    if ( needOpenLed == 1 )
    {
        kdrv_i2c_read_register( I2C_LED_DEVICE, LED_SLAVE_ID, 0x01, 1, 1, &led_status );
        led_status = led_status & (IO_LED1 | IO_LED2);
        data = data | led_status;
        kdrv_i2c_write_register( I2C_LED_DEVICE, LED_SLAVE_ID, 0x01, 1, 1, &data );  //set LED mode and turn on LED1 
    }
}
    
void nir_led_set_level(uint16_t level, io_led_index led_index)
{
    uint8_t nLevel, nLedBriReg;
    uint16_t data;
    
    if ( g_LedLightMode == LED_STANDBY_MODE )
    {
        led_set_light_mode(LED_TORCH_MODE);
    }

    if ( g_LedLightMode == LED_TORCH_MODE )
    {
        if( led_index == LED_CENTER ) nLedBriReg = 0x06;
        else nLedBriReg = 0x05;
    }
    else if ( g_LedLightMode == LED_FLASH_MODE )
    {
        if( led_index == LED_CENTER ) nLedBriReg = 0x04;
        else nLedBriReg = 0x03;
    }

    level = (level > 100)? (100):(level);
    nLevel = level * 255 / 100;  //mapping to 0~255

    nir_led_open( led_index );    //LED open check

    data = nLevel;
    kdrv_i2c_write_register( I2C_LED_DEVICE, LED_SLAVE_ID, nLedBriReg, 1, 1, &data);
    if( led_index == LED_BOTH )
    {
        kdrv_i2c_write_register( I2C_LED_DEVICE, LED_SLAVE_ID, nLedBriReg+1,  1, 1,  &data);
    }
}
void nir_led_close( io_led_index led_index ) 
{
    uint16_t data;
    
    led_set_light_mode( LED_TORCH_MODE );
    kdrv_i2c_read_register( I2C_LED_DEVICE, LED_SLAVE_ID, 0x01, 1, 1, (uint16_t*)&data );

    if ( led_index == LED_ONLY_ONE || led_index == LED_EDGE )
    {
        data = (data & IO_LED2) | g_LedLightMode;
        led_switch[LED_EDGE] = 0;
        
    }
    else if ( led_index == LED_CENTER )
    {
        data = (data & IO_LED1) | g_LedLightMode;
        led_switch[LED_CENTER] = 0;
    }
    else
    {
        data = g_LedLightMode;
        led_switch[LED_EDGE] = 0;
        led_switch[LED_CENTER] = 0;
    }
    
    kdrv_i2c_write_register( I2C_LED_DEVICE, LED_SLAVE_ID, 0x01, 1, 1, &data );  //Set to torch mode and turn off LED

}
void nir_led_init(uint16_t duty, io_led_index led_index)
{
    aw36515_init( LED_TORCH_MODE );
    if ( g_LedLightMode != LED_STANDBY_MODE )
    {
        nir_led_set_level( duty, led_index );
    }
}
#endif

static const struct sensor_win_size *ov02b1b_select_win(uint32_t *width, uint32_t *height)
{
    int i, default_size = ARRAY_SIZE(ov02b1b_supported_win_sizes) - 1;

    for (i = 0; i < ARRAY_SIZE(ov02b1b_supported_win_sizes); i++) {
        if (ov02b1b_supported_win_sizes[i].width  >= *width &&
            ov02b1b_supported_win_sizes[i].height >= *height) {
            *width = ov02b1b_supported_win_sizes[i].width;
            *height = ov02b1b_supported_win_sizes[i].height;
            return &ov02b1b_supported_win_sizes[i];
        }
    }

    *width = ov02b1b_supported_win_sizes[default_size].width;
    *height = ov02b1b_supported_win_sizes[default_size].height;
    return &ov02b1b_supported_win_sizes[default_size];
}


#if 1//(IMGSRC_0_SENSORID == SENSOR_ID_OV02B1B_R) || (IMGSRC_1_SENSORID == SENSOR_ID_OV02B1B_R)
static int ov02b1b_write_reg(struct sensor_device *sensor_dev, uint16_t reg, uint8_t data)
{
    int ret;
    kdrv_i2c_ctrl_t i2c_port = (kdrv_i2c_ctrl_t)sensor_dev->i2c_port;
    uint16_t dev_addr = sensor_dev->addr;

    ret = kdrv_i2c_write_register(i2c_port, dev_addr, reg, 1, 1, (uint16_t *)&data);
    
    return ret;
}
static int ov02b1b_read_reg(struct sensor_device *sensor_dev, uint16_t reg, uint8_t *data)
{
    int ret;
    kdrv_i2c_ctrl_t i2c_port = (kdrv_i2c_ctrl_t)sensor_dev->i2c_port;
    uint16_t dev_addr = sensor_dev->addr;

    ret = kdrv_i2c_read_register(i2c_port, dev_addr, reg, 1, 1, (uint16_t *)data);
    return ret;
}

int ov02b1b_r_init(struct sensor_device *sensor_dev, struct sensor_init_seq *seq)
{

    struct sensor_device *dev = sensor_dev;
    struct sensor_init_seq *init_fnc_ptr;
    sensor_msg(" ov02b1b_init");
    sensor_msg(" ov02b1b_init i2c_port:%d   addr:%d" , dev->i2c_port , dev->addr);
  /*
  uint8_t z=0;
  for(z=1;z<255;z++)
  {
    kdrv_i2c_write_register(dev->i2c_port, z, 0x02, 1, 1, 0x78);
  }
  */
    ov02b1b_write_reg(dev, 0xfc , 0x01);//soft reset

    for (init_fnc_ptr = seq; ; ++init_fnc_ptr) {
        if(init_fnc_ptr->addr == 0 && init_fnc_ptr->value == 0) break; //reaches end
        ov02b1b_write_reg(dev, init_fnc_ptr->addr , (uint8_t)(init_fnc_ptr->value & 0xFF));
    }

    ov02b1b_write_reg(dev, 0xfd , 0x00); 
    ov02b1b_write_reg(dev, 0x36 , 0x00);
    ov02b1b_write_reg(dev, 0x37 , 0x0d);
    ov02b1b_write_reg(dev, 0xfd , 0x01); 
    ov02b1b_write_reg(dev, 0x94 , 0x00);//output frame immediately
    ov02b1b_write_reg(dev, 0x95 , 0xc8);//output one frame per valid fsin
    //stream on    
    ov02b1b_write_reg(dev, 0xfd , 0x03);
    ov02b1b_write_reg(dev, 0xc2 , 0x01);
    ov02b1b_write_reg(dev, 0xfd , 0x01);

    ov02b1b_write_reg(dev, 0xfd , 0x00);
    
    sensor_msg("ov02b1b_init init over, sensor ID = 0x%x", dev->device_id);
    uint8_t data = 0;
    ov02b1b_read_reg(dev, 0x02, &data);
    sensor_msg("ov02b1b_init sensor high id=%x", data);
    dev->device_id = data << 8;
    ov02b1b_read_reg(dev, 0x03, &data);
    sensor_msg("ov02b1b_init sensor low id=%x", data);
    dev->device_id |= data;
    sensor_msg("ov02b1b_init init over, sensor ID = 0x%x\n", dev->device_id);
    return 0;
}


static int ov02b1b_r_set_params(
        struct sensor_device *sensor_dev, uint32_t *width, uint32_t *height, uint32_t fourcc)
{
    return 0;
}

static kdev_status_t kdev_sensor_r_init(void)
{
    sensor_msg("   <%s>\n", __func__);
    ov02b1b_r_init(&ov02b1b_dev[DEV_RIGHT], ov02b1b_init_regs); 
    return KDEV_STATUS_OK;
}

static kdev_status_t kdev_sensor_r_set_fmt(cam_format *fmt)
{
    //sensor_msg("   <%s>\n", __func__);

    ov02b1b_select_win(&fmt->width, &fmt->height);

    return (kdev_status_t)ov02b1b_r_set_params(&ov02b1b_dev[DEV_RIGHT], &fmt->width, &fmt->height, fmt->pixelformat);
}

int kdev_sensor_r_set_aec_roi(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, uint8_t center_x1, uint8_t center_x2, uint8_t center_y1, uint8_t center_y2)
{
    // not support
    return KDEV_STATUS_OK;
}
kdev_status_t kdev_sensor_r_get_lux(uint16_t* exposure, uint8_t* pre_gain_h, uint8_t* pre_gain_l, uint8_t* global_gain, uint8_t* y_average)
{
    // not support
    return KDEV_STATUS_OK;
}

kdev_status_t kdev_sensor_r_set_gain(uint32_t ana_gn1, uint32_t ana_gn2)
{    
    struct sensor_device *dev = &ov02b1b_dev[DEV_RIGHT];

    ov02b1b_write_reg( dev, 0xfd, 0x01 );
    ov02b1b_write_reg( dev, 0x22, (ana_gn2&0xFF) );
    ov02b1b_write_reg( dev, 0xfd, 0x01 );
    ov02b1b_write_reg( dev, 0xFE, 0x02 );

    return KDEV_STATUS_OK;
}

static int kdev_sensor_r_sleep( bool enable )
{
    // not supprot
    return KDEV_STATUS_OK;
}

kdev_status_t kdev_sensor_r_set_exp_time(uint32_t expo_time)
{
    struct sensor_device *dev = &ov02b1b_dev[DEV_RIGHT];

    ov02b1b_write_reg( dev, 0xfd, 0x01 );
    ov02b1b_write_reg( dev, 0x0E, ((expo_time&0xFF00)>>8) );
    ov02b1b_write_reg( dev, 0x0F, (expo_time&0xFF) );
    ov02b1b_write_reg( dev, 0xfd, 0x01 );
    ov02b1b_write_reg( dev, 0xFE, 0x02 );

    return KDEV_STATUS_OK;
}

static kdev_status_t kdev_sensor_r_set_devaddress(uint32_t address, uint32_t port_id)
{
    ov02b1b_dev[DEV_RIGHT].addr = address;
    ov02b1b_dev[DEV_RIGHT].i2c_port = port_id;
    return KDEV_STATUS_OK;
}

static uint32_t kdev_sensor_r_get_dev_id(void)
{
    return ov02b1b_dev[DEV_RIGHT].device_id;
}

static struct sensor_ops ov02b1b_r_ops = {
    .set_fmt            = kdev_sensor_r_set_fmt,
    .set_gain           = kdev_sensor_r_set_gain,
    .set_exp_time       = kdev_sensor_r_set_exp_time,
    .get_lux            = kdev_sensor_r_get_lux,
    .set_aec_roi        = kdev_sensor_r_set_aec_roi,
    .set_mirror         = NULL,
    .set_flip           = NULL,
    .get_dev_id         = kdev_sensor_r_get_dev_id,
    .set_fps            = NULL,
    .sleep              = kdev_sensor_r_sleep,
    .set_addr           = kdev_sensor_r_set_devaddress,
    .init               = kdev_sensor_r_init,
};
#endif

#if 1//(IMGSRC_0_SENSORID == SENSOR_ID_OV02B1B_L)||(IMGSRC_1_SENSORID == SENSOR_ID_OV02B1B_L)
//static int ov02b1b_l_write_reg(struct sensor_device *sensor_dev, uint16_t reg, uint8_t data)
//{
//    int ret;
//    kdrv_i2c_ctrl_t i2c_port = (kdrv_i2c_ctrl_t)ov02b1b_dev[DEV_LEFT].i2c_port;
//    uint16_t dev_addr = ov02b1b_dev[DEV_LEFT].addr;

//    ret = kdrv_i2c_write_register(i2c_port, dev_addr, reg, 1, 1, (uint16_t *)&data);

//    return ret;
//}
//static int ov02b1b_l_read_reg(struct sensor_device *sensor_dev, uint16_t reg, uint8_t *data)
//{
//    int ret;
//    kdrv_i2c_ctrl_t i2c_port = (kdrv_i2c_ctrl_t)ov02b1b_dev[DEV_LEFT].i2c_port;
//    uint16_t dev_addr = ov02b1b_dev[DEV_LEFT].addr;

//    ret = kdrv_i2c_read_register(i2c_port, dev_addr, reg, 1, 1, (uint16_t *)data);

//    return ret;
//}

int ov02b1b_l_init(struct sensor_device *sensor_dev, struct sensor_init_seq *seq)
{
    struct sensor_device *dev = sensor_dev;
    struct sensor_init_seq *init_fnc_ptr;
    sensor_msg(" ov02b1b_init");
    sensor_msg(" ov02b1b_init i2c_port:%d   addr:%d" , dev->i2c_port , dev->addr);
    ov02b1b_write_reg(dev, 0xfc , 0x01);//soft reset

    for (init_fnc_ptr = seq; ; ++init_fnc_ptr) {
        if(init_fnc_ptr->addr == 0 && init_fnc_ptr->value == 0) break; //reaches end
        ov02b1b_write_reg(dev, init_fnc_ptr->addr , (uint8_t)(init_fnc_ptr->value & 0xFF));
    }

    ov02b1b_write_reg(dev, 0xfd , 0x00); 
    ov02b1b_write_reg(dev, 0x1d , 0x00);
    ov02b1b_write_reg(dev, 0x36 , 0x00);
    ov02b1b_write_reg(dev, 0x37 , 0x03);
    ov02b1b_write_reg(dev, 0xfd , 0x01);
    ov02b1b_write_reg(dev, 0x92 , 0x0a);
    ov02b1b_write_reg(dev, 0x93 , 0x01);//fsin output every frame
    ov02b1b_write_reg(dev, 0x90 , 0x10);
    
        //stream on    
    ov02b1b_write_reg(dev, 0xfd , 0x03);
    ov02b1b_write_reg(dev, 0xc2 , 0x01);
    ov02b1b_write_reg(dev, 0xfd , 0x01);

    sensor_msg("ov02b1b_init init over, sensor ID = 0x%x", dev->device_id);
    uint8_t data = 0;
    ov02b1b_read_reg(dev, 0x02, &data);
    sensor_msg("ov02b1b_init sensor high id=%x", data);
    dev->device_id = data << 8;
    ov02b1b_read_reg(dev, 0x03, &data);
    sensor_msg("ov02b1b_init sensor low id=%x", data);
    dev->device_id |= data;
    sensor_msg("ov02b1b_init init over, sensor ID = 0x%x\n", dev->device_id);

    return 0;
}
static int ov02b1b_l_set_params(
        struct sensor_device *sensor_dev, uint32_t *width, uint32_t *height, uint32_t fourcc)
{
    return 0;
}
static kdev_status_t kdev_sensor_l_init(void)
{
    sensor_msg("   <%s>\n", __func__);
    ov02b1b_l_init(&ov02b1b_dev[DEV_LEFT], ov02b1b_init_regs);
    return KDEV_STATUS_OK;
}
static kdev_status_t kdev_sensor_l_set_fmt(cam_format *fmt)
{
    //sensor_msg("   <%s>\n", __func__);

    ov02b1b_select_win(&fmt->width, &fmt->height);

    return (kdev_status_t)ov02b1b_l_set_params(&ov02b1b_dev[DEV_LEFT], &fmt->width, &fmt->height, fmt->pixelformat);
}

int kdev_sensor_l_set_aec_roi(uint8_t x1, uint8_t x2, uint8_t y1, uint8_t y2, uint8_t center_x1, uint8_t center_x2, uint8_t center_y1, uint8_t center_y2)
{
    // not support
    return 0;
}
kdev_status_t kdev_sensor_l_get_lux(uint16_t* exposure, uint8_t* pre_gain_h, uint8_t* pre_gain_l, uint8_t* global_gain, uint8_t* y_average)
{
    // not support
    return KDEV_STATUS_OK;
}

kdev_status_t kdev_sensor_l_set_gain(uint32_t ana_gn1, uint32_t ana_gn2)
{
    struct sensor_device *dev = &ov02b1b_dev[DEV_LEFT];

    ov02b1b_write_reg( dev, 0xfd, 0x01 );
    ov02b1b_write_reg( dev, 0x22, (ana_gn2&0xFF) );
    ov02b1b_write_reg( dev, 0xfd, 0x01 );
    ov02b1b_write_reg( dev, 0xFE, 0x02 );


    return KDEV_STATUS_OK;
}

kdev_status_t kdev_sensor_l_set_exp_time(uint32_t expo_time)
{
    struct sensor_device *dev = &ov02b1b_dev[DEV_LEFT];

    ov02b1b_write_reg( dev, 0xfd, 0x01 );
    ov02b1b_write_reg( dev, 0x0E, ((expo_time&0xFF00)>>8) );
    ov02b1b_write_reg( dev, 0x0F, (expo_time&0xFF) );
    ov02b1b_write_reg( dev, 0xfd, 0x01 );
    ov02b1b_write_reg( dev, 0xFE, 0x02 );

    return KDEV_STATUS_OK;
}


static int kdev_sensor_l_sleep( bool enable )
{
    // not support
    return KDEV_STATUS_OK;
}
static kdev_status_t kdev_sensor_l_set_devaddress(uint32_t address, uint32_t port_id)
{
    ov02b1b_dev[DEV_LEFT].addr = address;
    ov02b1b_dev[DEV_LEFT].i2c_port = port_id;
    return KDEV_STATUS_OK;
}

static uint32_t kdev_sensor_l_get_dev_id(void)
{
    return ov02b1b_dev[DEV_LEFT].device_id;
}


static struct sensor_ops ov02b1b_l_ops = {
    .set_fmt            = kdev_sensor_l_set_fmt,
    .set_gain           = kdev_sensor_l_set_gain,
    .set_exp_time       = kdev_sensor_l_set_exp_time,
    .get_lux            = kdev_sensor_l_get_lux,
    .set_aec_roi        = kdev_sensor_l_set_aec_roi,
    .set_mirror         = NULL,
    .set_flip           = NULL,
    .get_dev_id         = kdev_sensor_l_get_dev_id,
    .set_fps            = NULL,
    .sleep              = kdev_sensor_l_sleep,
    .set_addr           = kdev_sensor_l_set_devaddress,
    .init               = kdev_sensor_l_init,
};
#endif
kdev_status_t kdev_sensor_fsync(void)
{
    return KDEV_STATUS_OK;
}
#if (IMGSRC_0_SENSORID == SENSOR_ID_OV02B1B_R)
KDEV_CAM_SENSOR_DRIVER_REGISTER(cam_sensor, 0, &ov02b1b_r_ops);
#endif
#if (IMGSRC_1_SENSORID == SENSOR_ID_OV02B1B_R)
KDEV_CAM_SENSOR_DRIVER_REGISTER(cam_sensor, 1, &ov02b1b_r_ops);
#endif

#if (IMGSRC_0_SENSORID == SENSOR_ID_OV02B1B_L)
KDEV_CAM_SENSOR_DRIVER_REGISTER(cam_sensor, 0, &ov02b1b_l_ops);
#endif
#if (IMGSRC_1_SENSORID == SENSOR_ID_OV02B1B_L)
KDEV_CAM_SENSOR_DRIVER_REGISTER(cam_sensor, 1, &ov02b1b_l_ops);
#endif


#endif
