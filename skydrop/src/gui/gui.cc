#include "gui.h"
#include "../drivers/buzzer.h"
#include "widgets/widgets.h"

#include "pages.h"
#include "splash.h"

#include "settings/settings.h"
#include "settings/set_vario.h"
#include "settings/set_audio.h"
#include "gui_value.h"
#include "settings/set_widgets.h"
#include "settings/layouts.h"
#include "settings/set_layout.h"
#include "settings/set_display.h"
#include "usb.h"
#include "factory_test.h"
#include "settings/set_system.h"
#include "settings/set_autostart.h"
#include "settings/set_gps.h"
#include "settings/set_gps_detail.h"
#include "settings/set_debug.h"


n5110display disp;
CreateStdOut(lcd_out, disp.Write);

volatile uint8_t gui_task = GUI_NONE;
volatile uint8_t gui_new_task = GUI_SPLASH;

void (* gui_init_array[])() =
	{gui_pages_init, gui_settings_init, gui_splash_init, gui_set_vario_init, gui_value_init,
	gui_set_audio_init, gui_set_widgets_init, gui_layouts_init, gui_set_layout_init,
	gui_set_display_init, gui_usb_init, gui_factory_test_init, gui_set_system_init,
	gui_set_autostart_init, gui_set_gps_init, gui_set_gps_detail_init, gui_set_debug_init};

void (* gui_stop_array[])() =
	{gui_pages_stop, gui_settings_stop, gui_splash_stop, gui_set_vario_stop, gui_value_stop,
	gui_set_audio_stop, gui_set_widgets_stop, gui_layouts_stop, gui_set_layout_stop,
	gui_set_display_stop, gui_usb_stop, gui_factory_test_stop, gui_set_system_stop,
	gui_set_autostart_stop, gui_set_gps_stop, gui_set_gps_detail_stop, gui_set_debug_stop};

void (* gui_loop_array[])() =
	{gui_pages_loop, gui_settings_loop, gui_splash_loop, gui_set_vario_loop, gui_value_loop,
	gui_set_audio_loop, gui_set_widgets_loop, gui_layouts_loop, gui_set_layout_loop,
	gui_set_display_loop, gui_usb_loop, gui_factory_test_loop, gui_set_system_loop,
	gui_set_autostart_loop, gui_set_gps_loop, gui_set_gps_detail_loop, gui_set_debug_loop};

void (* gui_irqh_array[])(uint8_t type, uint8_t * buff) =
	{gui_pages_irqh, gui_settings_irqh, gui_splash_irqh, gui_set_vario_irqh, gui_value_irqh,
	gui_set_audio_irqh, gui_set_widgets_irqh, gui_layouts_irqh, gui_set_layout_irqh,
	gui_set_display_irqh, gui_usb_irqh, gui_factory_test_irqh, gui_set_system_irqh,
	gui_set_autostart_irqh, gui_set_gps_irqh, gui_set_gps_detail_irqh, gui_set_debug_irqh};

#define GUI_ANIM_STEPS	20

void gui_switch_task(uint8_t new_task)
{
	gui_new_task = new_task;
}


uint8_t lcd_brightness;
uint8_t lcd_brightness_timeout;
uint32_t lcd_brightness_end = 0;
uint8_t lcd_contrast;
uint8_t lcd_contrast_min;
uint8_t lcd_contrast_max;
volatile bool lcd_new_contrast = false;

void gui_trigger_backlight()
{
	if (lcd_brightness == 0 || lcd_brightness_timeout == 0)
		lcd_bckl(0);
	else
	{
		lcd_bckl(lcd_brightness);
		lcd_brightness_end = task_get_ms_tick() + lcd_brightness_timeout * 1000;
	}
}

void gui_set_contrast(uint8_t contrast)
{
	lcd_contrast = contrast;
	lcd_new_contrast = true;
}

char gui_message[32];
uint32_t gui_message_end = 0;
#define MESSAGE_DURATION	5

void gui_showmessage_P(const char * msg)
{
	strcpy_P(gui_message, msg);
	gui_message_end = task_get_ms_tick() + MESSAGE_DURATION * 1000ul;
}

void gui_showmessage(char * msg)
{
	strcpy(gui_message, msg);
	gui_message_end = task_get_ms_tick() + MESSAGE_DURATION * 1000ul;
}

void gui_raligh_text(char * text, uint8_t x, uint8_t y)
{
	disp.GotoXY(x - disp.GetTextWidth(text), y);
	fprintf_P(lcd_out, PSTR("%s"), text);
}

void gui_caligh_text(char * text, uint8_t x, uint8_t y)
{
	disp.GotoXY(x - disp.GetTextWidth(text) / 2, y);
	fprintf_P(lcd_out, PSTR("%s"), text);
}


void gui_init()
{
	disp.Init();
	gui_load_eeprom();
}

void gui_load_eeprom()
{
	eeprom_busy_wait();
	lcd_brightness = eeprom_read_byte(&config.gui.brightness);
	DEBUG("lcd_brightness %d\n", lcd_brightness);
	if (lcd_brightness == 0xFF) lcd_brightness = 100;

	lcd_brightness_timeout = eeprom_read_byte(&config.gui.brightness_timeout);
	DEBUG("lcd_brightness_timeout %d\n", lcd_brightness_timeout);
	if (lcd_brightness_timeout == 0xFF) lcd_brightness_timeout = 3;

	lcd_contrast = eeprom_read_byte(&config.gui.contrast);
	lcd_contrast_min = eeprom_read_byte(&config_ro.lcd_contrast_min);
	lcd_contrast_max = eeprom_read_byte(&config_ro.lcd_contrast_max);
	DEBUG("lcd_contrast %d\n", lcd_contrast);

	if (lcd_contrast == 0xFF)
		lcd_contrast = 72;


	disp.SetContrast(lcd_contrast);
}

void gui_stop()
{
	disp.Stop();
}

uint8_t asdf = 0;
uint8_t fps_counter = 0;
uint8_t fps_val = 0;
uint32_t fps_timer = 0;

uint32_t gui_loop_timer = 0;

void gui_dialog(char * title)
{
	disp.LoadFont(F_TEXT_M);
	gui_caligh_text(title, GUI_DISP_WIDTH / 2, 2);

	disp.InvertPart(0, 0, n5110_width, 0);
	disp.PutPixel(0, 0, 0);
	disp.PutPixel(n5110_width - 1, 0 ,0);
	disp.Invert(0, 8, n5110_width, 11);

	disp.DrawLine(0, 12, 0, n5110_height - 2, 1);
	disp.DrawLine(n5110_width - 1, 12, n5110_width - 1, n5110_height - 2, 1);
	disp.DrawLine(1, n5110_height - 1, n5110_width - 2, n5110_height - 1, 1);
}

//disp layers
// 0 - layer to draw
// 1 - help layer

extern uint32_t task_timer_high;

uint8_t cont = 0;

void gui_loop()
{
	if (gui_loop_timer > task_get_ms_tick())
		return;

//	gui_loop_timer = (uint32_t)task_get_ms_tick() + (uint32_t)40; //25 fps
	gui_loop_timer = (uint32_t)task_get_ms_tick() + (uint32_t)50; //20 fps

	if (lcd_new_contrast)
	{
		//need to be outside of IRQ
		lcd_new_contrast = false;
		disp.SetContrast(lcd_contrast);
	}

	if (gui_new_task != gui_task)
	{
		if (gui_task != GUI_NONE)
			gui_stop_array[gui_task]();

		gui_task = gui_new_task;
		buttons_reset();
		gui_init_array[gui_task]();
	}

	disp.ClearBuffer();

	gui_loop_array[gui_task]();

	if (gui_message_end > task_get_ms_tick())
	{
		disp.LoadFont(F_TEXT_M);
		uint8_t w = disp.GetTextWidth(gui_message);
		uint8_t h = disp.GetAHeight();
		uint8_t x = GUI_DISP_WIDTH / 2 - w / 2;
		uint8_t y = GUI_DISP_HEIGHT / 2 - h / 2;
		uint8_t pad = 3;
		disp.DrawRectangle(x - 1 - pad, y - 1 - pad, x + w + pad, y + h + pad, 0, 1);

		disp.DrawLine(x - pad,			y - 2 - pad, 		x + w + pad, 		y - 2 - pad, 		1);
		disp.DrawLine(x - pad, 			y + h + 1 + pad, 	x + w + pad, 		y + h + 1 + pad, 	1);

		disp.DrawLine(x - 1 - pad, 		y - 1 - pad, 		x - 1 - pad, 		y + h + pad, 	1);
		disp.DrawLine(x + w + 1 + pad, 	y - 1 - pad, 		x + w + 1 + pad, 	y + h + pad, 	1);
		disp.GotoXY(x, y);
		fprintf_P(lcd_out, PSTR("%s"), gui_message);
	}

	// FPS counter

//	fps_counter++;
//
//	if (fps_timer < task_get_ms_tick())
//	{
//		fps_val = fps_counter;
//		fps_counter = 0;
//		fps_timer = task_get_ms_tick() + 1000;
//	}
//
//	disp.LoadFont(font_6px_normal_ttf_8);
//	disp.GotoXY(1, 1);
//	disp.ClearPart(0, 0, 1, 10);
//	fprintf_P(lcd_out, PSTR("%d"), fps_val);

	// FPS end

	disp.Draw();

	if (gui_task != GUI_SPLASH)
		if (buttons_read(B_LEFT) || buttons_read(B_RIGHT) || buttons_read(B_MIDDLE))
			gui_trigger_backlight();

	if (lcd_brightness_end < task_get_ms_tick())
		lcd_bckl(0);
}

void gui_force_loop()
{
	if (lcd_new_contrast)
	{
		//need to be outside of IRQ
		lcd_new_contrast = false;
		disp.SetContrast(lcd_contrast);
	}

	disp.ClearBuffer();

	if (gui_new_task != gui_task)
	{
		if (gui_task != GUI_NONE)
			gui_stop_array[gui_task]();
		gui_task = gui_new_task;
		gui_init_array[gui_task]();
	}

	gui_loop_array[gui_task]();

	disp.Draw();
}

void gui_irqh(uint8_t type, uint8_t * buff)
{
	if (type == B_LEFT || type == B_MIDDLE || type == B_RIGHT)
		gui_message_end = task_get_ms_tick();

	switch(type)
	{

	default:
		if (gui_task != GUI_NONE)
			gui_irqh_array[gui_task](type, buff);
	}
}

void gui_statusbar()
{
	//GPS indicator
	if (fc.use_gps)
	{
		char tmp[3];
		disp.LoadFont(F_TEXT_S);
		sprintf_P(tmp, PSTR("G"));
		if(fc.gps_data.valid)
		{
			gui_raligh_text(tmp, GUI_DISP_WIDTH, 0);
		}
		else
		{
			if (GUI_BLINK_TGL(1000))
				gui_raligh_text(tmp, GUI_DISP_WIDTH, 0);
		}
	}

	//battery indicator
	uint8_t a = battery_per / 10;

	disp.DrawLine(GUI_DISP_WIDTH - 5, GUI_DISP_HEIGHT - 13, GUI_DISP_WIDTH - 2, GUI_DISP_HEIGHT - 13, 1);
	disp.DrawRectangle(GUI_DISP_WIDTH - 6, GUI_DISP_HEIGHT - 12, GUI_DISP_WIDTH - 1, GUI_DISP_HEIGHT - 1, 1, 0);
	disp.DrawRectangle(GUI_DISP_WIDTH - 5, GUI_DISP_HEIGHT - 1 - a, GUI_DISP_WIDTH - 2, GUI_DISP_HEIGHT - 1, 1, 1);

}
