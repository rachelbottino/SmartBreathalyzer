#include "asf.h"
#include "image.h"
#include "ioport.h"
#include "logo.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


/** ASCII char definition for backspace. */
#define ASCII_BS    0x7F
/** ASCII char definition for carriage return. */
#define ASCII_CR    13

#define STRING_EOL    "\r\n"
#define STRING_HEADER "-- SAME70 LCD DEMO --"STRING_EOL	\
	"-- "BOARD_NAME " --"STRING_EOL	\
	"-- Compiled: "__DATE__ " "__TIME__ " --"STRING_EOL
//NEW
/** Reference voltage for AFEC,in mv. */
#define VOLT_REF        (3300)

/** The maximal digital value */
/** 2^12 - 1                  */
#define MAX_DIGITAL     (4095UL)

/** The conversion data is done flag */
volatile bool is_conversion_done = false;

/** The conversion data value */
volatile uint32_t g_ul_value = 0;

/* Canal do sensor MQ3 */
#define AFEC_CHANNEL_ALC_SENSOR 6
//NEW.
struct ili9488_opt_t g_ili9488_display_opt;

/**
}
/**
 * \brief Configure UART console.
 */
//NEW
/************************************************************************/
/* Funcoes                                                              */
/************************************************************************/

/**
 * \brief Configure UART console.
 * BaudRate : 115200
 * 8 bits
 * 1 stop bit
 * sem paridade
 */

static void configure_console(void)
{
 
	const usart_serial_options_t uart_serial_options = {
		.baudrate   = CONF_UART_BAUDRATE,
		.charlength = CONF_UART_CHAR_LENGTH,
		.paritytype = CONF_UART_PARITY,
		.stopbits   = CONF_UART_STOP_BITS,
	};

	/* Configure console UART. */
	sysclk_enable_peripheral_clock(CONSOLE_UART_ID);
	stdio_serial_init(CONF_UART, &uart_serial_options);
}

static float convert_adc_to_conc(int32_t ADC_value){
  
	int32_t ul_vol;
	float ul_conc;
	float rs_air;
	float r0;
	float rs_gas;
	float ratio;
  
	ul_vol = (ADC_value * VOLT_REF / MAX_DIGITAL);
	rs_air = (VOLT_REF-1060)/1060;
	r0 = rs_air/80.0; 	
	rs_gas = (((float) VOLT_REF)-((float) ul_vol)) /((float) ul_vol); 
	ratio = rs_gas/r0;  // ratio = RS/R0
		
	return (ul_vol);
}
/**
 * \brief AFEC interrupt callback function.
 */
static void AFEC_Temp_callback(void)
{
	g_ul_value = afec_channel_get_value(AFEC1, AFEC_CHANNEL_ALC_SENSOR);
	is_conversion_done = true;
}
//NEW.
/**
 * \brief Main application function.
 *
 * Initialize system, UART console, network then start weather client.
 *
 * \return Program return value.
 */
int main(void)
{
	/* Initialize the board. */
	sysclk_init();
	board_init();
	ioport_init();
	
	/* Initialize the UART console. */
	configure_console();
	printf(STRING_HEADER);
//NEW
	/* inicializa delay */
	delay_init(sysclk_get_cpu_hz());
	 /************************************* 
   * Ativa e configura AFEC
   *************************************/  

  /* Ativa AFEC - 1 */
	afec_enable(AFEC1);

  /* struct de configuracao do AFEC */
	struct afec_config afec_cfg;

  /* Carrega parametros padrao */
	afec_get_config_defaults(&afec_cfg);

  /* Configura AFEC */
	afec_init(AFEC1, &afec_cfg);
  
  /* Configura trigger por software */
  afec_set_trigger(AFEC1, AFEC_TRIG_SW);
  
  /* configura call back */
 	afec_set_callback(AFEC1, AFEC_INTERRUPT_EOC_6,	AFEC_Temp_callback, 1); 
   
  /*** Configuracao específica do canal AFEC ***/
  struct afec_ch_config afec_ch_cfg;
  afec_ch_get_config_defaults(&afec_ch_cfg);
  afec_ch_cfg.gain = AFEC_GAINVALUE_0;
  afec_ch_set_config(AFEC1, AFEC_CHANNEL_ALC_SENSOR, &afec_ch_cfg);
  
  /*
   * Calibracao:
	 * Because the internal ADC offset is 0x200, it should cancel it and shift
	 * down to 0.
	 */
	afec_channel_set_analog_offset(AFEC1, AFEC_CHANNEL_ALC_SENSOR, 0x200);

  /* Selecina canal e inicializa conversão */  
	afec_channel_enable(AFEC1, AFEC_CHANNEL_ALC_SENSOR);
	afec_start_software_conversion(AFEC1);

	while (1) {
		if(is_conversion_done == true) {
			is_conversion_done = false;
//NEW.	
	/* Set direction and pullup on the given button IOPORT */
	ioport_set_pin_dir(GPIO_PUSH_BUTTON_1, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(GPIO_PUSH_BUTTON_1, IOPORT_MODE_PULLUP);

	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_BLACK);//cor do quadrado
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_BLACK);//cor do quadrado
	//formatar int usando sprintf

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLUEVIOLET));//cor do plano de fundo
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, 120-1);//retangulo de cima do texto
	ili9488_draw_filled_rectangle(0, 360, ILI9488_LCD_WIDTH-1, 480-1);//retangulo de baixo do texto
	//ili9488_draw_pixmap(100, 120, 100, 100, logoImage);//imagem(x,y da posicao x, y do tamanho)

    /* Escreve na tela */
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_BLUE));//bacground da caixa de escrita
	ili9488_draw_filled_rectangle(0, 300, ILI9488_LCD_WIDTH-1, 315);//retangulo para escrever
	ili9488_set_foreground_color(COLOR_CONVERT(COLOR_WHITE));// cor da letra
	ili9488_draw_string(100, 300, (uint8_t *)"ALCOHOL %");// o que está escrito e sua posicao
	ili9488_draw_string(15, 100, (uint8_t *)"INTELLIGENT BREATHALYZER");// o que está escrito e sua posicao
		unsigned int tensao = convert_adc_to_conc(g_ul_value);   // example value
		unsigned char buffer[7];      // buffer to put string
		//int pb= sprintf(buffer," %d", tensao);
		itoa(tensao, buffer, 10);    // make the string
		//lcd_puts(buffer);             // show the string
		ili9488_draw_string(140, 210, (uint8_t *)buffer);// o que está escrito e sua posicao
while (1) {
	
	}
	return 0;
		}
	}
}